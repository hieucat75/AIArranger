#!/usr/bin/env python3
"""Unit test for the Gate 3 latency report tool.

Runs entirely on the synthetic sample_trace.json — NO hardware. Asserts the
percentile / segment / by-tempo / hot-plug math produces the known-correct
numbers, and that the end-to-end CLI exits PASS on the sample.

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


def main():
    records, counters = lr.load_trace(SAMPLE)
    rep = lr.build_report(records, counters, lr.DEFAULT_THRESHOLDS)

    # ── Percentile primitive (documented nearest-rank) ────────────────
    s = sorted([8, 9, 10, 12, 20])
    check("nearest-rank p50 of 5", lr.percentile(s, 50) == 10)
    check("nearest-rank p95 of 5", lr.percentile(s, 95) == 20)
    check("nearest-rank p100 == max", lr.percentile(s, 100) == 20)

    # ── Journeys ──────────────────────────────────────────────────────
    check("7 journeys assembled", len(rep["journeys"]) == 7)

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

    # ── verdict PASS on sample ────────────────────────────────────────
    check("sample verdict is PASS", rep["verdict_ok"] is True)

    # ── CLI end-to-end exits 0 (PASS) on the sample ───────────────────
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = lr.main([SAMPLE])
    out = buf.getvalue()
    check("CLI exit code 0 (PASS)", rc == 0)
    check("CLI prints PASS verdict", "result    : PASS" in out)
    check("CLI warns software != hardware pass",
          "software-green" in out.lower() or "not a hardware pass" in out.lower())

    # ── A failing threshold flips the verdict ─────────────────────────
    strict = {"p50_ms": 5.0, "p95_ms": 25.0, "max_ms": 40.0}
    rep2 = lr.build_report(records, counters, strict)
    check("tighter p50 threshold -> FAIL", rep2["verdict_ok"] is False)

    print(f"\n{'ALL PASSED' if failures == 0 else str(failures) + ' FAILED'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
