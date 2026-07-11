#!/usr/bin/env python3
"""Unit test for the Gate 3 latency report tool.

Runs entirely on synthetic data — NO hardware. Two layers:
  1. The bundled sample_trace.json exercises the percentile / segment / by-tempo /
     hot-plug math (7 known-correct journeys). With the min-sample gate that
     7-journey sample is now INSUFFICIENT_DATA, which is itself asserted.
  2. Programmatically synthesized journeys exercise the data-sufficiency gates:
     the minimum-sample boundary (0 / 1 / 19 / 20), the dropped-records block, and
     incomplete-correlation handling (missing stages excluded from percentiles).

Run:  python3 test_latency_report.py   (exit 0 = all passed)
"""

import io
import os
import sys
import contextlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import latency_report as lr  # noqa: E402

SAMPLE = os.path.join(HERE, "sample_trace.json")

failures = 0


def check(name, cond):
    global failures
    if cond:
        print(f"  PASS: {name}")
    else:
        print(f"  FAIL: {name}")
        failures += 1


def approx(a, b, eps=1e-6):
    return abs(a - b) <= eps


# ── Synthetic-journey helpers (data-sufficiency gate tests) ───────────

CLEAN_COUNTERS = {
    "input_callbacks": 0, "output_events": 0, "midi_sends": 0,
    "dropped_records": 0, "active_notes": 0, "panics": 0, "reconnects": 0,
    "queue_overflows": 0, "late_ticks": 0,
}


def _norm(ts_ns, corr, stage, tempo=120, tag="none"):
    return lr._normalize({
        "timestamp_ns": ts_ns, "correlation_id": corr, "stage": stage,
        "tempo_bpm": tempo, "lifecycle_tag": tag,
    })


def synth_records(n_complete, n_incomplete=0):
    """Build a records list.

    Each complete journey has all five stages and a chord→output of 10 ms (well
    within the default thresholds). Each incomplete journey is identical except
    it is MISSING accomp_first_event — it still has chord + enqueue + send, so a
    naive tool would fold it into the percentile; the gate must instead exclude
    and count it. Journeys are 1 s apart so nearest-preceding input matching is
    unambiguous."""
    recs = []
    corr = 0
    for k in range(n_complete + n_incomplete):
        is_complete = k < n_complete
        corr += 1
        base = (k + 1) * 1_000_000_000
        recs.append(_norm(base, 0, "input_note_on"))
        recs.append(_norm(base + 3_000_000, corr, "chord_confirmed",
                          tag="chord_change"))
        if is_complete:
            recs.append(_norm(base + 7_000_000, corr, "accomp_first_event"))
        recs.append(_norm(base + 8_000_000, corr, "output_enqueue"))
        recs.append(_norm(base + 13_000_000, corr, "midi_send"))
    return recs


def verdict_for(n_complete, n_incomplete=0, counters=None):
    counters = CLEAN_COUNTERS if counters is None else counters
    recs = synth_records(n_complete, n_incomplete)
    return lr.build_report(recs, counters, lr.DEFAULT_THRESHOLDS)


# ── Sample-math tests ─────────────────────────────────────────────────

def test_sample_math():
    records, counters = lr.load_trace(SAMPLE)
    rep = lr.build_report(records, counters, lr.DEFAULT_THRESHOLDS)

    # ── Percentile primitive (documented nearest-rank) ────────────────
    s = sorted([8, 9, 10, 12, 20])
    check("nearest-rank p50 of 5", lr.percentile(s, 50) == 10)
    check("nearest-rank p95 of 5", lr.percentile(s, 95) == 20)
    check("nearest-rank p100 == max", lr.percentile(s, 100) == 20)

    # ── Journeys ──────────────────────────────────────────────────────
    check("7 journeys assembled", len(rep["journeys"]) == 7)
    check("all 7 sample journeys are complete",
          rep["complete_correlations"] == 7 and
          rep["incomplete_correlations"] == 0)

    # ── chord→output (the Gate metric): [8,9,10,12,20,11,13] ──────────
    gate = rep["chord_to_output"]
    check("chord_to_output count == 7", gate["count"] == 7)
    check("chord_to_output min == 8ms", approx(gate["min_ms"], 8.0))
    check("chord_to_output p50 == 11ms", approx(gate["p50_ms"], 11.0))
    check("chord_to_output p95 == 20ms", approx(gate["p95_ms"], 20.0))
    check("chord_to_output max == 20ms", approx(gate["max_ms"], 20.0))

    # ── input→chord: every journey is 3ms ─────────────────────────────
    itc = rep["segments"]["input_to_chord"]
    check("input_to_chord count == 7", itc["count"] == 7)
    check("input_to_chord p50 == 3ms", approx(itc["p50_ms"], 3.0))

    # ── chord→scheduler: every journey is 4ms ─────────────────────────
    cts = rep["segments"]["chord_to_scheduler"]
    check("chord_to_scheduler p50 == 4ms", approx(cts["p50_ms"], 4.0))

    # ── end→end: input→send = chord_to_output + 3ms ───────────────────
    e2e = rep["segments"]["end_to_end"]
    check("end_to_end max == 23ms", approx(e2e["max_ms"], 23.0))

    # ── by tempo ──────────────────────────────────────────────────────
    by_tempo = rep["by_tempo"]
    check("two tempo buckets", set(by_tempo.keys()) == {90, 120})
    check("120bpm bucket has 5", by_tempo[120]["count"] == 5)
    check("120bpm p50 == 10ms", approx(by_tempo[120]["p50_ms"], 10.0))
    check("120bpm max == 20ms", approx(by_tempo[120]["max_ms"], 20.0))
    check("90bpm bucket has 2", by_tempo[90]["count"] == 2)
    check("90bpm p50 == 11ms", approx(by_tempo[90]["p50_ms"], 11.0))

    # ── hot-plug split: 5 before, 2 after ─────────────────────────────
    before, after, hp = rep["hotplug"]
    check("hot-plug marker found", hp is not None)
    check("5 journeys before hot-plug", before["count"] == 5)
    check("2 journeys after hot-plug", after["count"] == 2)

    # ── counters / stuck notes ────────────────────────────────────────
    check("active_notes == 0 (no stuck notes)", counters["active_notes"] == 0)
    check("dropped_records == 0", counters["dropped_records"] == 0)
    check("queue_overflows == 0", counters["queue_overflows"] == 0)

    # ── verdict: 7 < 20 samples ⇒ INSUFFICIENT_DATA (never PASS) ───────
    check("7-journey sample verdict is INSUFFICIENT_DATA",
          rep["verdict"] == lr.VERDICT_INSUFFICIENT)
    check("INSUFFICIENT_DATA is not a PASS", rep["verdict_ok"] is False)

    # ── CLI end-to-end exits non-zero and prints the verdict label ────
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = lr.main([SAMPLE])
    out = buf.getvalue()
    check("CLI exit code 2 (not PASS)", rc == 2)
    check("CLI prints INSUFFICIENT_DATA verdict",
          "result    : INSUFFICIENT_DATA" in out)
    check("CLI warns software != hardware pass",
          "software-green" in out.lower() or "not a hardware pass" in out.lower())

    # ── A failing threshold flips the verdict to FAIL (hard breach) ───
    strict = {"p50_ms": 5.0, "p95_ms": 25.0, "max_ms": 40.0}
    rep2 = lr.build_report(records, counters, strict)
    check("tighter p50 threshold -> FAIL", rep2["verdict"] == lr.VERDICT_FAIL)

    # ── Dropped trace records force FAIL even on the sample ────────────
    dropped = dict(counters, dropped_records=5000)
    rep3 = lr.build_report(records, dropped, lr.DEFAULT_THRESHOLDS)
    check("dropped_records > 0 -> FAIL (percentiles unreliable)",
          rep3["verdict"] == lr.VERDICT_FAIL)


# ── Minimum-sample gate: 0 / 1 / 19 / 20 boundary ─────────────────────

def test_min_sample_gate():
    r0 = verdict_for(0)
    check("0 samples -> not PASS", r0["verdict_ok"] is False)
    check("0 samples -> INSUFFICIENT_DATA", r0["verdict"] == lr.VERDICT_INSUFFICIENT)
    check("0 samples -> gate count 0", r0["chord_to_output"].get("count", 0) == 0)

    r1 = verdict_for(1)
    check("1 sample -> not PASS", r1["verdict_ok"] is False)
    check("1 sample -> INSUFFICIENT_DATA", r1["verdict"] == lr.VERDICT_INSUFFICIENT)

    r19 = verdict_for(19)
    check("19 samples -> not PASS", r19["verdict_ok"] is False)
    check("19 samples -> INSUFFICIENT_DATA",
          r19["verdict"] == lr.VERDICT_INSUFFICIENT)
    check("19 samples -> gate count 19", r19["chord_to_output"]["count"] == 19)

    r20 = verdict_for(20)  # 20 complete, 0 dropped
    check("20 samples, 0 drop -> PASS", r20["verdict"] == lr.VERDICT_PASS)
    check("20 samples -> verdict_ok True", r20["verdict_ok"] is True)
    check("20 samples -> gate count 20", r20["chord_to_output"]["count"] == 20)

    # 20 valid samples but a dropped record still blocks (FAIL, not PASS).
    r20d = verdict_for(20, counters=dict(CLEAN_COUNTERS, dropped_records=1))
    check("20 samples but dropped>0 -> FAIL",
          r20d["verdict"] == lr.VERDICT_FAIL and r20d["verdict_ok"] is False)


# ── Incomplete-correlation reporting ──────────────────────────────────

def test_incomplete_correlation():
    # 20 complete + 3 incomplete: the 3 missing-stage correlations are counted
    # and reported, NOT folded into the percentile (gate count stays 20), and
    # 3/23 = 13% exceeds the 5% allowance -> INSUFFICIENT_DATA.
    r = verdict_for(20, n_incomplete=3)
    check("incomplete counted (3)", r["incomplete_correlations"] == 3)
    check("total correlations 23", r["total_correlations"] == 23)
    check("complete correlations 20", r["complete_correlations"] == 20)
    check("incomplete NOT in percentile (gate count 20)",
          r["chord_to_output"]["count"] == 20)
    check("excess incomplete -> INSUFFICIENT_DATA (not PASS)",
          r["verdict"] == lr.VERDICT_INSUFFICIENT and r["verdict_ok"] is False)
    check("incomplete_fraction ~= 3/23",
          approx(r["incomplete_fraction"], 3 / 23))

    # A small share of incomplete (1/41 = 2.4% < 5%) is reported but does not
    # block: with enough complete samples and clean counters this still PASSes.
    r2 = verdict_for(40, n_incomplete=1)
    check("small incomplete share is reported", r2["incomplete_correlations"] == 1)
    check("small incomplete share still PASSes",
          r2["verdict"] == lr.VERDICT_PASS)
    check("PASS percentile excludes the incomplete one (count 40)",
          r2["chord_to_output"]["count"] == 40)


def main():
    test_sample_math()
    test_min_sample_gate()
    test_incomplete_correlation()
    print(f"\n{'ALL PASSED' if failures == 0 else str(failures) + ' FAILED'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
