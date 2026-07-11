# Gate 3 — Latency Instrumentation Harness

> **Scope:** an RT-safe latency **measurement** harness for the live play path. It
> adds trace points, a lock-free trace ring, a non-RT collector, Gate-3 counters,
> and an offline report tool. It **does not change engine timing** — every trace
> point is a compiled-out no-op by default and, when enabled, only does a
> wait-free ring push. Default is **OFF** (production zero-overhead).
>
> **This harness alone cannot mark Gate 3 PASS.** Software timestamps are
> necessary but not sufficient — see [§8 Hardware caveat](#8-hardware-caveat) and
> [GATE3_HARDWARE_VALIDATION_CHECKLIST.md](GATE3_HARDWARE_VALIDATION_CHECKLIST.md).
> `hardware_validated:false` stays in place regardless of any software-green run.

---

## 1. What it measures

The five waypoints of a single input→output journey (causal order):

| # | Trace stage id       | Where it fires                                            | Thread            |
|---|----------------------|-----------------------------------------------------------|-------------------|
| 1 | `input_note_on`      | `CoreMidiIn::readProc` (`src/engine/midi/coremidi_in.cpp`) | CoreMIDI read thread |
| 2 | `chord_confirmed`    | `StylePlayer::setChord` (`src/engine/arranger/style_player.cpp`) | control/engine thread |
| 3 | `accomp_first_event` | `StylePlayer::dispatchSectionEvents` (`src/engine/arranger/style_player.cpp`) | audio tick thread |
| 4 | `output_enqueue`     | `CoreMidiOut::send` (`src/engine/midi/coremidi_out.cpp`)   | audio tick thread |
| 5 | `midi_send`          | `CoreMidiOut::dispatchOne` (`src/engine/midi/coremidi_out.cpp`) | MIDI dispatch thread |

Derived latency segments (computed in the report tool, **never** on the RT path):

- **input→chord** = `chord_confirmed − input_note_on`
- **chord→scheduler** = `accomp_first_event − chord_confirmed`
- **scheduler→output-send** = `midi_send − accomp_first_event`
- **chord→output** = `midi_send − chord_confirmed`  ← the Gate-3 budget metric
- **end-to-end** = `midi_send − input_note_on`

For each: sample count, p50, p95, max, jitter (stddev), a histogram, outliers,
latency by tempo, and latency before/after a hot-plug.

Correlation: a fresh `correlation_id` is stamped at **chord confirm** (stage 2)
and carried by stages 3–5, so those group cleanly. The triggering NoteOn (stage
1) carries the *previous* id, so the report tool pairs each chord with the
nearest preceding input NoteOn temporally.

## 2. Architecture

```
 RT producers (read / audio / dispatch threads)
   AIARR_TRACE_*  --->  LatencyTracer::emit()  --->  MpscRing<TraceRecord, 2^16>
                          (wait-free, drop+count on full - never blocks)
                                                          |
 non-RT consumer                                          v
   TraceCollector (background thread OR manual drainNow)  drains ring
                          |
                          v
                 CSV / JSON  --->  tools/latency_report/latency_report.py
```

- **`src/engine/trace/trace_record.h`** — `TraceRecord`, a 30-byte packed POD
  (`static_assert`-checked, trivially copyable). All string-ish dimensions are
  small enum/ids (`TraceStage`, `LifecycleTag`, device ids); **no `std::string`,
  no pointers** in the record.
- **`src/engine/trace/trace_clock.h`** — `traceNowNs()`: monotonic-raw ns.
  `mach_absolute_time` on Apple, `clock_gettime(CLOCK_MONOTONIC_RAW)` elsewhere.
- **`src/engine/trace/mpsc_ring.h`** — bounded lock-free **MPSC** ring (Vyukov).
  Generalises the repo's single-producer `LockFreeQueue` to the multiple RT
  producer threads the waypoints run on. Full ring → `try_push` returns false
  (drop), never blocks.
- **`src/engine/trace/latency_trace.{h,cpp}`** — the process-wide `LatencyTracer`
  singleton: `emit()`, correlation ids, RT context (tempo/section/device), the
  Gate-3 counters, and the `AIARR_TRACE_*` macros.
- **`src/engine/trace/trace_collector.{h,cpp}`** — non-RT drainer + CSV/JSON
  exporter (background thread or synchronous `drainNow()`).

### Record fields (CSV/JSON columns)
`timestamp_ns, correlation_id, stage, channel, note, velocity, chord_root,
chord_type, section, tempo_bpm, input_device_id, output_device_id, lifecycle_tag`

## 3. Gate-3 counters (atomic, read off-RT)

Exposed via `tracer().counters()` and written to the JSON export:

`input_callbacks`, `output_events`, `midi_sends`, `dropped_records` (ring-full
trace drops), `active_notes` (gauge — must be 0 after stop/panic/switch/close =
no stuck notes), `panics`, `reconnects` (disconnect/reconnect), `queue_overflows`
(output/scheduler queue rejects), `late_ticks` (late-tick / catch-up).

## 4. Enabling / disabling

**Default is OFF** — every `AIARR_TRACE_*` macro is `((void)0)`, no ring is
allocated, no trace symbols are emitted (verified: `nm libarranger-engine.a`
shows no `LatencyTracer` symbols in an OFF build).

Two independent gates:

1. **Build gate (primary, zero-overhead):**
   ```sh
   cmake -S . -B build -DAIARR_LATENCY_TRACE=ON
   cmake --build build -j
   ```
2. **Runtime gate (debug builds only):** once compiled in, toggle without a
   rebuild:
   ```cpp
   ai_arranger::trace::tracer().setEnabled(false); // emit() early-returns
   ```

Do **not** ship production with `-DAIARR_LATENCY_TRACE=ON`.

## 5. Capturing a trace (real hardware run)

With a trace-enabled build of `AI Arranger.app`:

```cpp
#include "engine/trace/trace_collector.h"
using ai_arranger::trace::TraceCollector;

TraceCollector collector(/*pollMicros=*/1000);
collector.start();               // begins draining the ring off-RT
// ... run the live session (play chords, switch sections, hot-plug, soak) ...
collector.stop();                // final drain + join
collector.writeJson("session.json");   // records + counters
collector.writeCsv("session.csv");     // records only
```

The collector runs on a normal thread; only the ring is touched from RT.

## 6. Report / analysis tool

`tools/latency_report/latency_report.py` — pure Python 3 stdlib, **no hardware,
no build step**. Reads a CSV or JSON export.

```sh
python3 tools/latency_report/latency_report.py session.json
# or try it against the bundled synthetic sample:
python3 tools/latency_report/latency_report.py tools/latency_report/sample_trace.json
```

It prints per-segment p50/p95/max/jitter, a chord→output histogram, outliers,
latency by tempo, latency before/after hot-plug, the lifecycle counters, and a
PASS/FAIL against the thresholds below. Exit code `0` = software PASS, `2` =
software FAIL. Percentiles use the nearest-rank method (documented in-file).

Unit test: `python3 tools/latency_report/test_latency_report.py` (asserts the math
on the sample). Ring/tracer unit test: the C++ `test_latency_trace_ring` target.

## 7. Provisional Gate-3 thresholds

Applied to **chord→output** under normal conditions (mirrored as the report
tool's defaults):

| Metric | Budget |
|--------|--------|
| p50    | ≤ 15 ms |
| p95    | ≤ 25 ms |
| max    | ≤ 40 ms |
| stuck notes | 0 (`active_notes` == 0 after stop/panic/switch/close) |
| crashes / deadlocks | 0 |
| `queue_overflows` | 0 |
| soak 60 min | no cumulative drift, counters advance monotonically |

## 8. Hardware caveat

**A green software report is NOT a Gate 3 PASS.** Timestamps prove the software
path is within budget; they say nothing about what the sound module actually
does. A real pass **must** combine, on physical gear:

1. **Software trace** (this harness) within the §7 thresholds, AND
2. **Listening / observing the sound module** — the accompaniment is audibly in
   time and re-voices correctly, AND
3. **Hot-plug** of input and output devices (no crash, no stuck note), AND
4. **App close** while playing (module silenced before exit), AND
5. **Soak 30–60 min** with balanced NoteOn/NoteOff and no drift.

All of these are enumerated in
[GATE3_HARDWARE_VALIDATION_CHECKLIST.md](GATE3_HARDWARE_VALIDATION_CHECKLIST.md).
`hardware_validated:false` remains until every box there is checked on real
hardware. This harness makes step 1 measurable; it does not replace steps 2–5.
