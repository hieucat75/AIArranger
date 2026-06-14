# Gate 11 — Plan: KORG Validation Harness (pre-hardware)

> **PROPOSAL ONLY — no code in this document.** Implements the
> "Required Infrastructure Before Hardware Acquisition" of
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md`. Hardware remains
> **DEFERRED_BY_PTH**; this gate builds the *measurement* tooling so the day a
> Korg PA700/PA1000 arrives is measurement, not tooling.

---

## 1. Goal

Deliver a **measurable, repeatable, CI-safe validation harness** under
`tools/korg_validation/` that compares our engine's MIDI behaviour against a
reference capture — proving section timing, fill alignment, chord latency,
stuck-note safety, and jitter **deterministically with synthetic fixtures**, and
ready to ingest a real Korg capture later with zero code changes.

## 2. Out of scope (status rules)

- ❌ **No hardware parity claim.** A green harness on synthetic fixtures proves the
  *tooling and the engine against a reference*, NOT parity with a real device.
- ❌ **No KORG compatibility claim** until real PA700/PA1000 capture evidence is
  committed (per `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`).
- ✅ **Software harness PASS allowed** ("validation harness green on synthetic
  fixtures").
- ❌ No real-device I/O in CI; no CoreMIDI device calls in tests.

## 3. Architecture / file layout

```
tools/korg_validation/
├── CMakeLists.txt
├── include/korg_validation/
│   ├── midi_capture.h          # capture model + loader (SMF/CSV -> timed events)
│   ├── timing_analyzer.h       # per-event delta engine-vs-reference
│   ├── transition_validator.h  # section/fill boundary correctness
│   ├── latency_meter.h         # chord-in -> first re-voiced note
│   ├── stuck_note_detector.h   # NoteOn/NoteOff balance + hanging notes
│   ├── jitter_benchmark.h      # inter-event timing error distribution
│   └── reporter.h              # JSON + Markdown emit
├── src/                        # one .cpp per header
└── tests/                      # CI-safe, synthetic fixtures only
fixtures/korg/
├── README.md                   # capture format + naming convention
├── PA700/                      # placeholder (intended real captures later)
│   └── .gitkeep
└── PA1000/
    └── .gitkeep
```

The harness links the existing `arranger-engine` (to generate the engine-side
event stream) and is **read-only** w.r.t. the engine — no engine code changes.

## 4. Per-component design (10 scope items)

### 4.1 `tools/korg_validation/` directory + CMake
- **Input:** none (build scaffolding).
- **Output:** a `korg-validation` library + `korg-validate` CLI + test targets.
- **Approach:** new CMake subdir, globbed like `importer-sff1`; tests registered
  with CTest so they join `scripts/run_tests.sh`.
- **CI-safe test:** target builds and links on macos-latest.
- **Acceptance:** `cmake --build` produces the lib + CLI + tests; CTest sees them.

### 4.2 MIDI capture input pipeline (`midi_capture.h`)
- **Input:** a capture file (SMF `.mid` or CSV of `tick,status,d1,d2`) — both our
  engine output and a reference device output use the same schema.
- **Output:** `std::vector<TimedMidiEvent>` (tick/ms, status, data) — a normalized
  in-memory stream.
- **Approach:** reuse the SMF delta-parse logic pattern from `sff1_reader`; add a
  trivial CSV loader for hand-authored fixtures.
- **CI-safe test:** load a synthetic 2-bar fixture, assert event count + first/last
  ticks.
- **Acceptance:** SMF and CSV loaders yield identical normalized streams for an
  equivalent fixture.

### 4.3 Timing differential analyzer (`timing_analyzer.h`)
- **Input:** two normalized streams (engine, reference) + a match tolerance.
- **Output:** per-event `delta_ms` list + summary (mean/max/stddev, matched/orphan
  counts).
- **Approach:** align by (note, channel, order) within a window; report signed
  deltas; flag unmatched events.
- **CI-safe test:** reference == engine → all deltas 0; reference shifted +5ms →
  mean delta ≈ 5.
- **Acceptance:** correct deltas on identical, shifted, and missing-event fixtures.

### 4.4 Section / fill transition validator (`transition_validator.h`)
- **Input:** engine stream + expected section/fill boundary ticks (bar grid).
- **Output:** per-transition `{expected_tick, actual_tick, delta, on_boundary}`.
- **Approach:** detect section markers / first-event-of-section; compare to the
  bar grid (reuses Gate 10B intro-immediate + bar-quantize semantics).
- **CI-safe test:** fixture with a section switch on bar 2 → on_boundary=true;
  off-grid switch → flagged.
- **Acceptance:** correctly classifies on-boundary vs early/late for fills + main
  switches.

### 4.5 Chord response latency measurement (`latency_meter.h`)
- **Input:** a chord-change timestamp + the engine stream.
- **Output:** `latency_ticks` / `latency_ms` to the first re-voiced NoteOn after
  the change.
- **Approach:** scan for the first transposed NoteOn at/after the chord event;
  subtract timestamps.
- **CI-safe test:** synthetic chord change at tick T, first note at T+ticks →
  asserted latency.
- **Acceptance:** exact latency on fixtures; handles "no note after change" (∞/flag).

### 4.6 Panic / stuck-note detector (`stuck_note_detector.h`)
- **Input:** a normalized stream.
- **Output:** `{balanced, hanging_notes[], double_on[]}`.
- **Approach:** per-(channel,note) on/off counter; any net-on at end = hanging.
  Reuses the balance idea from `note_balance` / `test_panic`.
- **CI-safe test:** balanced fixture → clean; fixture with a missing NoteOff →
  one hanging note reported.
- **Acceptance:** detects hanging + double-on; passes a known-good stream.

### 4.7 Jitter benchmark (`jitter_benchmark.h`)
- **Input:** a normalized stream + an expected grid (e.g. 8th-note pulse).
- **Output:** jitter distribution (mean abs error, p95, max) of inter-event timing.
- **Approach:** snap each event to nearest grid slot, measure error; aggregate.
- **CI-safe test:** perfectly-quantized fixture → ~0 jitter; deliberately
  detuned fixture → bounded non-zero.
- **Acceptance:** stable, deterministic stats on fixed fixtures.

### 4.8 Report output: JSON + Markdown (`reporter.h`)
- **Input:** results from 4.3–4.7.
- **Output:** `report.json` (machine) + `report.md` (human) written to a path.
- **Approach:** hand-rolled JSON writer (no new deps); Markdown table template.
- **CI-safe test:** run harness on a fixture, assert JSON parses + has required
  keys; Markdown contains each section header.
- **Acceptance:** sample `report.json` + `report.md` committed to the repo.

### 4.9 Fixture set for Korg PA700/PA1000 (future test)
- **Input:** n/a (assets + docs).
- **Output:** `fixtures/korg/{PA700,PA1000}/` placeholders + a `README.md` defining
  the real-capture format and naming.
- **Approach:** synthetic engine-side fixtures live in `tools/korg_validation/tests/`;
  real-device slots stay empty (`.gitkeep`) with a documented schema.
- **CI-safe test:** README lints (paths referenced exist); placeholders present.
- **Acceptance:** clear, documented slot for real captures; nothing claims they
  exist.

### 4.10 CI-safe tests with synthetic MIDI fixtures
- **Input:** small in-repo fixtures (CSV/SMF) authored by hand.
- **Output:** CTest targets covering 4.2–4.8.
- **Approach:** every component test uses synthetic data only; no device, no
  network, no nondeterminism.
- **CI-safe test:** the whole harness suite runs under `scripts/run_tests.sh`.
- **Acceptance:** green on macos-latest hosted runner with no hardware.

## 5. Report format

**JSON schema (sketch):**
```json
{
  "harness_version": "1",
  "fixture": "synthetic_pop_2bar",
  "hardware_validated": false,
  "timing":     { "matched": 0, "orphan": 0, "mean_ms": 0.0, "max_ms": 0.0, "stddev_ms": 0.0 },
  "transitions":[ { "label": "Main A->B", "expected_tick": 0, "actual_tick": 0, "delta": 0, "on_boundary": true } ],
  "latency":    { "events": 0, "mean_ms": 0.0, "max_ms": 0.0 },
  "stuck":      { "balanced": true, "hanging": 0, "double_on": 0 },
  "jitter":     { "mean_abs_ms": 0.0, "p95_ms": 0.0, "max_ms": 0.0 }
}
```
> `"hardware_validated": false` is emitted **always** until real-device evidence
> exists — the report itself encodes the claims policy.

**Markdown template (sketch):**
```
# KORG Validation Report — <fixture>
> hardware_validated: false (synthetic fixture)
## Timing | ## Transitions | ## Chord latency | ## Stuck-note | ## Jitter
<one table per section>
```

## 6. Fixture spec

- **Format:** SMF (`.mid`) for device captures; CSV (`tick,status,data1,data2`)
  for hand-authored synthetic fixtures. Both normalize to `TimedMidiEvent`.
- **Naming:** `fixtures/korg/<MODEL>/<style>__<section>__<chord-seq>.mid`
  (e.g. `PA700/pop_acoustic__mainA__C-F-G-Am.mid`).
- **Synthetic fixtures** live beside the tests in `tools/korg_validation/tests/`.
- **Real captures** go in `fixtures/korg/PA700|PA1000/` — empty until hardware.

## 7. CI strategy

- New CTest targets auto-join the existing `scripts/run_tests.sh` (the workflow
  already runs that), so `.github/workflows/ctest.yml` needs **no change**.
- Synthetic-only: no CoreMIDI device access, no network → safe on macos-latest.
- The `korg-validate` CLI is buildable but **not** invoked against a device in CI.

## 8. Tasks breakdown (proposed commits)

| Task | Commit | Content |
|---|---|---|
| A | `G11-TASK-A` | scaffold `tools/korg_validation/` + CMake + CTest wiring + empty CLI (builds, no logic) |
| B | `G11-TASK-B` | `midi_capture` (SMF+CSV loader → normalized stream) + tests |
| C | `G11-TASK-C` | `timing_analyzer` (per-event delta + summary) + tests |
| D | `G11-TASK-D` | `transition_validator` + `latency_meter` + tests |
| E | `G11-TASK-E` | `stuck_note_detector` + `jitter_benchmark` + tests |
| F | `G11-TASK-F` | `reporter` (JSON+MD) + committed sample report + `korg-validate` CLI wires it all |
| G | `G11-TASK-G` | `fixtures/korg/` placeholders + README schema; synthetic fixture set |
| H | `G11-TASK-H` | `GATE_11_HANDOFF.md`; update HANDOFF_MASTER (status rules); open-items |

(6–8 commits, mirroring the G10/G10B one-task-per-commit cadence; tests green
after each.)

## 9. Acceptance criteria (Gate 11)

- All **10 scope items** have an implementation + CI-safe test.
- Entire harness suite runs green on the **macos-latest hosted runner with no
  hardware**.
- A **sample `report.json` + `report.md`** are committed (generated from a
  synthetic fixture).
- Docs explain **how to reactivate** with a real Korg (drop a capture in
  `fixtures/korg/PA700|PA1000/`, re-run `korg-validate`).
- **Status rules in handoff:** software harness PASS allowed; **hardware parity
  and KORG compatibility claims forbidden** until real PA evidence is committed.
  Every report emits `"hardware_validated": false`.
- Baseline suite stays green (22 binaries / 345 assertions) — harness adds new
  targets, changes no existing engine code.

## 10. Estimated effort

- **New source:** ~7 headers + ~7 impls + 1 CLI + 1 CMake ≈ 16 files.
- **New tests:** ~6–7 CTest binaries (one per component) + synthetic fixtures.
- **New docs/assets:** `GATE_11_HANDOFF.md`, `fixtures/korg/README.md`, sample
  reports, placeholders.
- **Complexity:** low–medium. The hard parts are *measurement correctness*
  (alignment/tolerance in the timing analyzer) and *determinism* (fixed
  fixtures, no clocks/RNG in tests). No realtime/audio-thread code → no RT-safety
  burden. No new third-party deps (hand-rolled JSON).
- **Risk:** low; fully synthetic + read-only against the engine.

---

> When approved, implement per the task breakdown — one commit per task, tests
> green after each, PR at the end. Do **not** claim hardware/KORG compatibility.
