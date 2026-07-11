#!/usr/bin/env python3
"""Gate 3 latency report / analysis CLI.

Reads a latency trace exported by the C++ harness (CSV or JSON — see
src/engine/trace/trace_collector.*) and prints the Gate-3 latency picture WITHOUT
any hardware: sample counts, p50/p95/max, jitter, histogram, outliers, latency by
tempo, latency before/after hot-plug, and the lifecycle counters (stuck notes /
drops / overflows). It then checks the provisional Gate-3 thresholds and prints a
PASS/FAIL verdict.

IMPORTANT: a green verdict here means the SOFTWARE timestamps are within budget.
It is NOT sufficient to mark Gate 3 PASS — see
docs/gate-plans/GATE_3_LATENCY_INSTRUMENTATION.md and
docs/gate-plans/GATE3_HARDWARE_VALIDATION_CHECKLIST.md: a real pass also needs
audible verification on the sound module, hot-plug, app-close, and a 30-60 min
soak.

Usage:
    python3 latency_report.py TRACE_FILE [--threshold-ms-p50 15] [--json]

Percentiles use the nearest-rank method on the sorted sample:
    rank(p) = ceil(p/100 * N)  (1-indexed), so p50 of 5 samples is the 3rd value.
This is deterministic and needs no numpy.
"""

import argparse
import bisect
import csv
import json
import math
import os
import sys
from collections import defaultdict

# ── Provisional Gate-3 thresholds (chord → output), milliseconds ──────
# Mirrors docs/gate-plans/GATE_3_LATENCY_INSTRUMENTATION.md §Thresholds.
DEFAULT_THRESHOLDS = {
    "p50_ms": 15.0,
    "p95_ms": 25.0,
    "max_ms": 40.0,
}

# Stage names as written by trace_collector (stageName()).
STAGE_INPUT = "input_note_on"
STAGE_CHORD = "chord_confirmed"
STAGE_ACCOMP = "accomp_first_event"
STAGE_ENQUEUE = "output_enqueue"
STAGE_SEND = "midi_send"

HOTPLUG_TAGS = {"hotplug_add", "hotplug_remove"}
LIFECYCLE_STUCK_CHECK_TAGS = {"stop", "panic", "section_switch", "app_close"}


# ── Loading ───────────────────────────────────────────────────────────

def load_trace(path):
    """Return (records, counters). counters is None for CSV input."""
    ext = os.path.splitext(path)[1].lower()
    if ext == ".json":
        with open(path) as f:
            doc = json.load(f)
        records = [_normalize(r) for r in doc.get("records", [])]
        counters = doc.get("counters")
        return records, counters
    # CSV
    records = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            records.append(_normalize(row))
    return records, None


def _normalize(r):
    """Coerce a raw record dict into typed fields."""
    def _int(k, default=0):
        v = r.get(k, default)
        try:
            return int(v)
        except (TypeError, ValueError):
            return default
    return {
        "timestamp_ns": _int("timestamp_ns"),
        "correlation_id": _int("correlation_id"),
        "stage": str(r.get("stage", "")).strip(),
        "channel": _int("channel"),
        "note": _int("note"),
        "velocity": _int("velocity"),
        "chord_root": _int("chord_root"),
        "chord_type": _int("chord_type"),
        "section": _int("section"),
        "tempo_bpm": _int("tempo_bpm"),
        "input_device_id": _int("input_device_id"),
        "output_device_id": _int("output_device_id"),
        "lifecycle_tag": str(r.get("lifecycle_tag", "none")).strip(),
    }


# ── Stats primitives ──────────────────────────────────────────────────

def percentile(sorted_vals, p):
    """Nearest-rank percentile of an already-sorted non-empty list."""
    n = len(sorted_vals)
    if n == 0:
        return float("nan")
    rank = max(1, math.ceil(p / 100.0 * n))
    return sorted_vals[min(rank, n) - 1]


def summarize(values):
    """p50/p95/max/min/mean/jitter(stddev) for a list of ms values."""
    if not values:
        return {"count": 0}
    s = sorted(values)
    n = len(s)
    mean = sum(s) / n
    var = sum((x - mean) ** 2 for x in s) / n
    return {
        "count": n,
        "min_ms": s[0],
        "p50_ms": percentile(s, 50),
        "p95_ms": percentile(s, 95),
        "max_ms": s[-1],
        "mean_ms": mean,
        "jitter_ms": math.sqrt(var),  # stddev
    }


def histogram(values, bins=10):
    """Return list of (lo, hi, count) buckets across the value range."""
    if not values:
        return []
    lo, hi = min(values), max(values)
    if hi == lo:
        return [(lo, hi, len(values))]
    width = (hi - lo) / bins
    counts = [0] * bins
    for v in values:
        idx = min(bins - 1, int((v - lo) / width))
        counts[idx] += 1
    return [(lo + i * width, lo + (i + 1) * width, counts[i]) for i in range(bins)]


# ── Journey assembly ──────────────────────────────────────────────────

def build_journeys(records):
    """Assemble per-correlation stage timestamps and pair each chord with the
    nearest preceding input NoteOn.

    Correlation ids are assigned at chord-confirm and shared by the downstream
    accomp/enqueue/send stages, so those group cleanly by correlation_id. The
    triggering input NoteOn carries the PREVIOUS correlation, so it is matched to
    a chord temporally (nearest input at-or-before the chord confirm)."""
    by_corr = defaultdict(dict)  # correlation_id -> stage -> earliest ts
    inputs = []                  # sorted input timestamps
    for r in records:
        st = r["stage"]
        if st == STAGE_INPUT:
            inputs.append(r["timestamp_ns"])
            continue
        cid = r["correlation_id"]
        if cid == 0:
            continue
        cur = by_corr[cid].get(st)
        if cur is None or r["timestamp_ns"] < cur:
            by_corr[cid][st] = r["timestamp_ns"]
    inputs.sort()

    journeys = []
    for cid, stages in by_corr.items():
        if STAGE_CHORD not in stages:
            continue
        chord_ts = stages[STAGE_CHORD]
        # nearest input at or before the chord confirm
        input_ts = _nearest_preceding(inputs, chord_ts)
        journeys.append({
            "correlation_id": cid,
            "tempo_bpm": _tempo_for(records, cid),
            "input_ts": input_ts,
            "chord_ts": chord_ts,
            "accomp_ts": stages.get(STAGE_ACCOMP),
            "enqueue_ts": stages.get(STAGE_ENQUEUE),
            "send_ts": stages.get(STAGE_SEND),
        })
    journeys.sort(key=lambda j: j["chord_ts"])
    return journeys


def _nearest_preceding(sorted_inputs, ts):
    i = bisect.bisect_right(sorted_inputs, ts) - 1
    return sorted_inputs[i] if i >= 0 else None


def _tempo_for(records, cid):
    for r in records:
        if r["correlation_id"] == cid and r["tempo_bpm"]:
            return r["tempo_bpm"]
    return 0


def _ms(a, b):
    """(b - a) in ms, or None if either endpoint is missing."""
    if a is None or b is None:
        return None
    return (b - a) / 1e6


def segment_latencies(journeys):
    """Return dict metric_name -> list of ms values."""
    out = {
        "input_to_chord": [],
        "chord_to_scheduler": [],        # chord_confirmed -> accomp_first_event
        "scheduler_to_output_send": [],  # accomp_first_event -> midi_send
        "chord_to_output": [],           # chord_confirmed -> midi_send (GATE metric)
        "end_to_end": [],                # input_note_on -> midi_send
    }
    for j in journeys:
        _append(out["input_to_chord"], _ms(j["input_ts"], j["chord_ts"]))
        _append(out["chord_to_scheduler"], _ms(j["chord_ts"], j["accomp_ts"]))
        _append(out["scheduler_to_output_send"], _ms(j["accomp_ts"], j["send_ts"]))
        _append(out["chord_to_output"], _ms(j["chord_ts"], j["send_ts"]))
        _append(out["end_to_end"], _ms(j["input_ts"], j["send_ts"]))
    return out


def _append(lst, v):
    if v is not None:
        lst.append(v)


def latency_by_tempo(journeys):
    """metric chord_to_output grouped by tempo_bpm -> summary."""
    groups = defaultdict(list)
    for j in journeys:
        v = _ms(j["chord_ts"], j["send_ts"])
        if v is not None:
            groups[j["tempo_bpm"]].append(v)
    return {tempo: summarize(vals) for tempo, vals in sorted(groups.items())}


def latency_around_hotplug(records, journeys):
    """Split chord_to_output by whether the send occurred before/after the first
    hot-plug marker. Returns (before_summary, after_summary, hotplug_ts)."""
    hotplug_ts = None
    for r in sorted(records, key=lambda x: x["timestamp_ns"]):
        if r["lifecycle_tag"] in HOTPLUG_TAGS:
            hotplug_ts = r["timestamp_ns"]
            break
    if hotplug_ts is None:
        return None, None, None
    before, after = [], []
    for j in journeys:
        v = _ms(j["chord_ts"], j["send_ts"])
        if v is None or j["send_ts"] is None:
            continue
        (before if j["send_ts"] < hotplug_ts else after).append(v)
    return summarize(before), summarize(after), hotplug_ts


# ── Reporting ─────────────────────────────────────────────────────────

def _fmt(summary):
    if not summary or summary.get("count", 0) == 0:
        return "no samples"
    return ("n={count} p50={p50_ms:.2f} p95={p95_ms:.2f} max={max_ms:.2f} "
            "mean={mean_ms:.2f} jitter={jitter_ms:.2f} ms").format(**summary)


def build_report(records, counters, thresholds):
    journeys = build_journeys(records)
    segs = segment_latencies(journeys)
    gate = summarize(segs["chord_to_output"])

    over_p50 = sum(1 for v in segs["chord_to_output"] if v > thresholds["p50_ms"])
    verdict_ok = True
    reasons = []
    if gate.get("count", 0) == 0:
        verdict_ok = False
        reasons.append("no chord→output samples")
    else:
        if gate["p50_ms"] > thresholds["p50_ms"]:
            verdict_ok = False
            reasons.append(f"p50 {gate['p50_ms']:.2f} > {thresholds['p50_ms']}")
        if gate["p95_ms"] > thresholds["p95_ms"]:
            verdict_ok = False
            reasons.append(f"p95 {gate['p95_ms']:.2f} > {thresholds['p95_ms']}")
        if gate["max_ms"] > thresholds["max_ms"]:
            verdict_ok = False
            reasons.append(f"max {gate['max_ms']:.2f} > {thresholds['max_ms']}")
    if counters:
        if counters.get("active_notes", 0) != 0:
            verdict_ok = False
            reasons.append(f"stuck notes: active_notes={counters['active_notes']}")
        if counters.get("dropped_records", 0) != 0:
            reasons.append(f"WARN dropped_records={counters['dropped_records']}")
        if counters.get("queue_overflows", 0) != 0:
            verdict_ok = False
            reasons.append(f"queue_overflows={counters['queue_overflows']}")

    return {
        "journeys": journeys,
        "segments": {k: summarize(v) for k, v in segs.items()},
        "raw_segments": segs,
        "chord_to_output": gate,
        "over_threshold_p50": over_p50,
        "by_tempo": latency_by_tempo(journeys),
        "hotplug": latency_around_hotplug(records, journeys),
        "counters": counters,
        "verdict_ok": verdict_ok,
        "verdict_reasons": reasons,
        "thresholds": thresholds,
    }


def print_report(rep):
    print("=" * 68)
    print("Gate 3 Latency Report  (SOFTWARE trace — not a hardware pass)")
    print("=" * 68)
    print(f"Journeys (chord→output correlations): {len(rep['journeys'])}")
    print()
    print("── Segment latencies ──────────────────────────────────────────")
    for name in ["input_to_chord", "chord_to_scheduler",
                 "scheduler_to_output_send", "chord_to_output", "end_to_end"]:
        print(f"  {name:26s}: {_fmt(rep['segments'][name])}")
    print()

    gate = rep["chord_to_output"]
    if gate.get("count", 0):
        print("── chord→output histogram (ms) ────────────────────────────────")
        for lo, hi, c in histogram(rep["raw_segments"]["chord_to_output"]):
            bar = "#" * c
            print(f"  [{lo:6.2f}, {hi:6.2f})  {c:3d} {bar}")
        outliers = sorted(v for v in rep["raw_segments"]["chord_to_output"]
                          if v > rep["thresholds"]["p50_ms"])
        print(f"  outliers > p50 threshold ({rep['thresholds']['p50_ms']} ms): "
              f"{['%.2f' % v for v in outliers] or 'none'}")
        print()

    print("── chord→output by tempo ──────────────────────────────────────")
    for tempo, summ in rep["by_tempo"].items():
        print(f"  {tempo:4d} bpm: {_fmt(summ)}")
    print()

    before, after, hp_ts = rep["hotplug"]
    print("── chord→output around hot-plug ───────────────────────────────")
    if hp_ts is None:
        print("  (no hot-plug marker in trace)")
    else:
        print(f"  before: {_fmt(before)}")
        print(f"  after : {_fmt(after)}")
    print()

    print("── Lifecycle counters ─────────────────────────────────────────")
    if rep["counters"]:
        c = rep["counters"]
        for k in ["input_callbacks", "output_events", "midi_sends",
                  "dropped_records", "active_notes", "panics", "reconnects",
                  "queue_overflows", "late_ticks"]:
            print(f"  {k:18s}: {c.get(k, 'n/a')}")
        print(f"  stuck notes after stop/panic/switch/close: "
              f"{'NONE (active_notes=0)' if c.get('active_notes', 0) == 0 else 'STUCK!'}")
    else:
        print("  (no counters — CSV input; supply JSON export for counters)")
    print()

    print("── Gate-3 verdict (SOFTWARE ONLY) ─────────────────────────────")
    th = rep["thresholds"]
    print(f"  thresholds: p50≤{th['p50_ms']} p95≤{th['p95_ms']} max≤{th['max_ms']} ms")
    print(f"  result    : {'PASS' if rep['verdict_ok'] else 'FAIL'}")
    if rep["verdict_reasons"]:
        for r in rep["verdict_reasons"]:
            print(f"    - {r}")
    print("  NOTE: software-green ≠ Gate 3 PASS. Requires hardware validation")
    print("        (sound module + hot-plug + app-close + 30-60min soak).")
    print("=" * 68)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Gate 3 latency report")
    ap.add_argument("trace_file", help="CSV or JSON trace export")
    ap.add_argument("--threshold-ms-p50", type=float,
                    default=DEFAULT_THRESHOLDS["p50_ms"])
    ap.add_argument("--threshold-ms-p95", type=float,
                    default=DEFAULT_THRESHOLDS["p95_ms"])
    ap.add_argument("--threshold-ms-max", type=float,
                    default=DEFAULT_THRESHOLDS["max_ms"])
    ap.add_argument("--json", action="store_true",
                    help="emit the report as JSON instead of text")
    args = ap.parse_args(argv)

    thresholds = {
        "p50_ms": args.threshold_ms_p50,
        "p95_ms": args.threshold_ms_p95,
        "max_ms": args.threshold_ms_max,
    }
    records, counters = load_trace(args.trace_file)
    rep = build_report(records, counters, thresholds)

    if args.json:
        printable = {k: v for k, v in rep.items() if k != "journeys"}
        print(json.dumps(printable, indent=2, default=str))
    else:
        print_report(rep)
    return 0 if rep["verdict_ok"] else 2


if __name__ == "__main__":
    sys.exit(main())
