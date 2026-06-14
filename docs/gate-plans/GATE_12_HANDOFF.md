# Gate 12 — Handoff (Realtime Performer Layer)

> Branch `gate-12-realtime-performer-layer` off `main` (`4443778`).
> **Status: ✅ Gate 12 Engine PASS (deterministic, synthetic).**
> 38 test binaries, 527 assertions, 0 failures.
> Realtime/control logic only — **no DSP, no audio rendering, no UI, no
> hardware**. Every report emits `deterministic: true` + `hardware_validated:
> false` (hard-coded). No KORG/Yamaha compatibility claimed. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Scope (8 items) — all delivered

| # | Item | Module(s) | Task |
|---|------|-----------|------|
| 1 | Sync Start / Sync Stop | `performance/sync_controller` | B |
| 2 | Fill Queueing System | `performance/fill_scheduler` | C |
| 3 | Variation Transition Engine | `performance/variation_manager` | D |
| 4 | Chord Scan Modes | `chord/` (4 detectors + framework) | E |
| 5 | Split / Lower / Manual Bass | `performance/split_router` | F |
| 6 | Realtime Control Surface | `control/` (events + SPSC) | G |
| 7 | Performer-Safe Panic & Recovery | `performance/performer_panic` | H |
| 8 | Realtime Latency Budget | `performance/latency_report` | I |
| — | Foundation: realtime state machine | `performance/realtime_state_machine` | A |

## 2. Commits

| Hash | Title |
|---|---|
| `847ddb2` | G12-TASK-A: realtime performer state machine foundation |
| `c9a6cb1` | G12-TASK-B: sync controller (arm-before-play, race-safe single-fire) |
| `c2e33df` | G12-TASK-C: fill scheduler (queue, collapse, single-fire) |
| `da655c7` | G12-TASK-D: variation manager (A/B/C/D, deterministic resolve) |
| `958f068` | G12-TASK-E: chord scan mode framework + 4 detectors |
| `6641895` | G12-TASK-F: split router (split point, lower, manual bass) |
| `e35b784` | G12-TASK-G: control surface abstraction (events + SPSC) |
| `c708e4e` | G12-TASK-H: performer panic + recovery (extends G9 panic) |
| `1300d59` | G12-TASK-I: latency budget benchmarks + reporter |

(+ this handoff commit = Task J.)

## 3. Architecture

A control/performer layer built ON TOP of the engine — `src/engine/{performance,
control,chord}/` join the `arranger-engine` library; `tests/realtime/*.cpp` join
CTest (so `scripts/run_tests.sh` + CI cover them). **No core engine behaviour
changed**; the only engine-core addition is reuse of `PanicHandler` for
`PerformerPanic`. All cross-thread state is `std::atomic` or a lock-free SPSC
ring; no alloc/lock/syscall on the realtime path.

## 4. Test evidence

| Test (new) | Assertions |
|---|--:|
| test_realtime_state_machine | 17 |
| test_sync_controller | 9 |
| test_fill_scheduler | 7 |
| test_variation_manager | 11 |
| test_chord_scan | 14 |
| test_split_router | 10 |
| test_control_surface | 7 |
| test_performer_panic | 9 |
| test_latency_budget | 13 |
| **Gate 12 new total** | **97** |

- Full suite: **38 binaries, 527 assertions, 0 failures** (was 29 / 430).
- Covers (per plan): rapid variation/fill spam, repeated panic recovery, split
  changes, sync-start race (8-thread), latency benchmarks, deterministic replay,
  SPSC producer/consumer.
- All synthetic; no hardware, no network. Baseline 29 binaries unchanged.

## 5. Sample report (committed)

`tests/realtime/sample/latency_report.{json,md}` — excerpt:
```json
{ "deterministic": true, "hardware_validated": false,
  "scenario": "synthetic_performer_steps", "all_within_budget": true,
  "metrics": [ { "name": "chord_detection", "max_ms": 0.000, "within_budget": true },
               { "name": "transition", "max_ms": 375.000, "budget_ms": 500.000 } ] }
```

## 6. Decisions worth noting

1. **Determinism via pure transition table + atomics.** The state machine is a
   pure `next()` table; identical input sequences replay identically (tested with
   two machines + multi-thread).
2. **Race-safe single-fire** (`tryApply`, CAS) for sync-start and fill/variation
   collapse — verified under 8-thread concurrency, not just rapid single-thread.
3. **Control surface is vendor-neutral + UI-free** — `ControlEvent`s only; device
   button maps belong in adapters, never in `src/engine/`.
4. **Modules are decision-only units**, deterministically unit-tested in
   isolation; they emit intent (queue/state) that the sequencer/scheduler act on
   — no scheduler bypass, no audio.
5. **Reports hard-code `deterministic:true` + `hardware_validated:false`** from
   constants; cannot accidentally claim hardware parity.
6. **Single-finger chord rule is a documented simplification**, not a
   vendor-specific spatial scheme; AI-fingered is an explicit placeholder (no
   model, falls back to Fingered).

## 7. Residual risks / status rules

- **Software/realtime PASS allowed** ("deterministic on synthetic control
  sequences"). **Hardware parity + KORG compatibility claims FORBIDDEN** until
  committed real-device evidence (Claims Policy). Reports stay
  `hardware_validated:false`.
- The layer is **not yet wired into a live MIDI input front-end** — adapters
  (real ControlSurface implementations) are future work; the engine-side seam is
  done.
- Latency is **logical-step / quantize latency**, not wall-clock device latency.
- Full StylePlayer integration (driving start/stop/section from these modules end
  to end) is intentionally minimal here — modules are decision units; an
  integrating coordinator is a follow-up.

## 8. Merge status

**MERGE_ALLOWED: NO** — pending PTH review (Gate discipline). Branch pushed; PR to
be opened. CI expected green (synthetic-only).
