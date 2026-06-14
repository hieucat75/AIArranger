# Gate 11 — Handoff (KORG Validation Harness, pre-hardware)

> Branch `gate-11-korg-validation-harness` off `main` (`ff915c3`).
> **Status: ✅ Gate 11 Harness PASS (synthetic) / 🅓 Hardware DEFERRED_BY_PTH.**
> 29 test binaries, 430 assertions, 0 failures.
> The harness runs on **synthetic fixtures only**. A green run proves the tooling
> + analyzers against a reference — it is **NOT** parity with any real device.
> **No KORG/Yamaha compatibility is claimed**; every report emits
> `hardware_validated: false`. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Scope — all 10 plan items delivered

| # | Item | Where |
|---|------|-------|
| 1 | `tools/korg_validation/` directory + CMake/CTest | Task A |
| 2 | MIDI capture pipeline (CSV + SMF) | Task B |
| 3 | Timing differential analyzer | Task C |
| 4 | Section/fill transition validator | Task D |
| 5 | Chord response latency meter | Task D |
| 6 | Panic/stuck-note detector | Task E |
| 7 | Jitter benchmark | Task E |
| 8 | Report output (JSON + Markdown) | Task F |
| 9 | Korg PA700/PA1000 fixture slots | Task G |
| 10 | CI-safe synthetic-fixture tests | A–G (every component) |

## 2. Commits

| Hash | Title |
|---|---|
| `5829b3b` | G11-TASK-A: scaffold tools/korg_validation harness (CMake + CTest + CLI) |
| `347f42d` | G11-TASK-B: MIDI capture pipeline (CSV + SMF -> normalized stream) |
| `dcaf9d1` | G11-TASK-C: timing differential analyzer |
| `4d73046` | G11-TASK-D: section/fill transition validator + chord latency meter |
| `2eb0d14` | G11-TASK-E: panic/stuck-note detector + jitter benchmark |
| `2c7745b` | G11-TASK-F: reporter (JSON + Markdown) + CLI wiring + sample report |
| `19e995a` | G11-TASK-G: Korg fixture placeholders + README schema + synthetic fixtures |

(+ this handoff commit = Task H.)

## 3. Architecture

`tools/korg_validation/` — a static lib `korg-validation` + CLI `korg-validate` +
per-component CTest binaries, added via `add_subdirectory` so its tests join
`scripts/run_tests.sh` and CI with no workflow change. The harness is a
**read-only consumer** — it makes **zero** changes to the engine.

```
tools/korg_validation/
├── include/korg_validation/{harness,midi_capture,timing_analyzer,
│        transition_validator,latency_meter,stuck_note_detector,
│        jitter_benchmark,reporter}.h
├── src/  (one .cpp per header)
├── korg_validate_cli.cpp
├── sample_report/{report.json,report.md}
└── tests/  (+ tests/fixtures/synthetic/*.csv)
fixtures/korg/{README.md, PA700/.gitkeep, PA1000/.gitkeep}
```

## 4. Test evidence

| Component test | Assertions |
|---|--:|
| test_harness_scaffold | 3 |
| test_midi_capture | 17 |
| test_timing_analyzer | 14 |
| test_transition_latency | 13 |
| test_stuck_jitter | 12 |
| test_reporter | 17 |
| test_fixtures | 9 |
| **Gate 11 new total** | **85** |

- Full suite: **29 binaries, 430 assertions, 0 failures** (was 22 / 345).
- All harness tests are **synthetic-only** — no device, no network, deterministic.
- Baseline engine suite (22 binaries / 345 assertions) unchanged; no engine code
  touched.

## 5. Sample report (committed)

`tools/korg_validation/sample_report/report.{json,md}` — generated from a
synthetic engine-vs-shifted-reference fixture. Excerpt:

```json
{
  "harness_version": 1,
  "hardware_validated": false,
  "fixture": "synthetic_two_bar (engine vs +5tick reference)",
  "timing": { "matched": 6, "orphan_engine": 0, "mean_ms": 0.868, "max_abs_ms": 5.208, ... },
  "stuck":  { "balanced": true, "hanging": 0, ... },
  "jitter": { "events": 3, "mean_abs_ms": 1.736, "max_ms": 5.208 }
}
```

## 6. Decisions worth noting

1. **`hardware_validated: false` is hard-coded** from a single constant
   (`harness.h::kHardwareValidated`); the report encodes the claims policy and
   cannot accidentally claim parity.
2. **CSV is the primary fixture format** (hand-authorable, deterministic); SMF is
   supported for future real captures and normalizes to the same stream.
3. **Order-preserving per-key match** in the timing analyzer makes identical
   streams give all-zero deltas and surfaces drift/drops cleanly.
4. **No engine changes**; harness links the engine read-only and lives entirely
   under `tools/` — not a realtime path, so no RT-safety burden.
5. **Capture pipeline reads files, not the live engine** (per the 10 scope
   items); a live-engine bridge is intentionally out of scope for Gate 11.

## 7. Reactivation (when hardware is available)

1. Record the Korg PA700/PA1000 MIDI OUT into `fixtures/korg/<MODEL>/`.
2. `korg-validate --engine <engine.csv|mid> --reference <device.mid> \
   --out-json report.json --out-md report.md`.
3. Commit captures + report; amend claims only then, via PR.

## 8. Residual risks / status rules

- **Software harness PASS allowed** ("green on synthetic fixtures").
- **Hardware parity claim FORBIDDEN**; **KORG compatibility claim FORBIDDEN**
  until committed real-PA capture evidence (Claims Policy). Reports stay
  `hardware_validated: false`.
- Synthetic fixtures are theory-authored, not device-derived — calibration
  against a real Korg is the future milestone.

## 9. Merge status

**MERGE_ALLOWED: NO** — pending PTH review (Gate discipline). Branch pushed; PR
to be opened. CI expected green (synthetic-only).
