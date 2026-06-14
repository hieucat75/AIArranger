# Gate 12 — Plan: Realtime Performer Layer (KORG-First)

> **PROPOSAL ONLY — no code in this document.** Builds the live-performance
> control layer on top of the existing engine (clock, sequencer, scheduler,
> panic, NTT, swing) and the Gate 11 validation harness.
>
> Authoring note: this plan was composed from the provided Gate 12 scope
> (the 8 item names, architectural constraints, forbidden list, suggested
> structure, testing/deliverable requirements) plus the current codebase. It
> claims nothing about hardware. Hardware remains **DEFERRED_BY_PTH**; every
> report this gate produces emits `deterministic: true` + `hardware_validated:
> false`. See `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Context

- **Current status:** Gates 1–11 merged. The engine plays UASF styles with
  per-role tracks, full NTR/NTT, swing, articulation strategy, immediate
  intro-start, panic-safe dispatch; a CI-safe KORG validation harness exists
  (synthetic-only, `hardware_validated=false`).
- **This phase is NOT about:** audio rendering / DSP, sound design, UI, or real
  Korg hardware. No device is required.
- **This phase IS about:** the **realtime performer layer** — the deterministic,
  low-latency control logic a player drives live (sync start/stop, fills,
  variations, chord scanning, split/manual bass, a vendor-neutral control
  surface, performer-grade panic/recovery, and a measured latency budget).

## 2. Strategic Direction

A **KORG-first realtime arranger experience**: the layer models how a performer
controls an arranger in real time, validated deterministically with synthetic
control sequences. The control surface is vendor-agnostic; Korg semantics inform
behaviour but no Korg/Yamaha compatibility is claimed. Identity remains a
software-defined arranger platform.

## 3. Scope (8 items)

### 3.1 Sync Start / Sync Stop
- **Requirements:** start playback on the first detected chord (sync-start armed);
  stop on a stop trigger or when the lower zone clears (sync-stop), quantized to
  the configured boundary (bar / beat / immediate).
- **Behavior target:** arming is instant; the actual start/stop lands on the next
  quantize boundary deterministically; re-arming is idempotent.
- **Do NOT:** start audio, allocate on the trigger path, or bypass the sequencer.

### 3.2 Fill Queueing System
- **Requirements:** queue a fill (auto / to-variation); fire at the next bar
  boundary; auto-return to the owning main (or advance to the target variation).
- **Behavior target:** rapid repeated fill presses collapse to one queued fill
  (no stacking); fill fires once, on the boundary.
- **Do NOT:** drop notes, double-fire, or leave the sequencer in a fill state.

### 3.3 Variation Transition Engine
- **Requirements:** switch Main A/B/C/D (and intro/ending) with quantized,
  deterministic transitions; support immediate vs next-bar policy.
- **Behavior target:** a queued variation applies exactly once at the boundary;
  late re-queues replace the pending target.
- **Do NOT:** transition mid-bar unless policy=immediate; leave stuck notes.

### 3.4 Chord Scan Modes
- **Requirements:** pluggable detectors — **Fingered**, **Fingered-on-Bass**,
  **Single Finger**, and an **AI Fingered placeholder** (interface only, no model).
- **Behavior target:** the same key input yields a deterministic `Chord` per mode;
  switching mode is glitch-free.
- **Do NOT:** claim AI behaviour (placeholder returns a documented fallback).

### 3.5 Split / Lower / Manual Bass
- **Requirements:** a split point partitions the keyboard; the lower zone feeds
  chord detection; a manual-bass mode lets the lowest lower-zone note override the
  bass.
- **Behavior target:** notes route deterministically by zone; manual bass wins
  over NTT bass when enabled.
- **Do NOT:** mis-route across the split; allocate per-note.

### 3.6 Realtime Control Surface Abstraction
- **Requirements:** a vendor-neutral interface for performer actions (start, stop,
  fill, variation select, intro, ending, tap, sync arm, panic) decoupled from any
  physical device.
- **Behavior target:** any front-end (MIDI, UI, test) emits the same
  `ControlEvent`s; the engine reacts identically.
- **Do NOT:** reference Korg/Yamaha button maps in the engine; couple to UI.

### 3.7 Performer-Safe Panic & Recovery
- **Requirements:** extend the Gate 9 panic handler for live use — all-notes-off,
  flush, and immediate clean re-arm without a restart.
- **Behavior target:** panic during any state leaves zero hanging notes and the
  layer ready to start again within the latency budget.
- **Do NOT:** block, allocate, or require a full reload to recover.

### 3.8 Realtime Latency Budget
- **Requirements:** define and measure a control-to-effect latency budget
  (control event → state change → first dispatched consequence).
- **Behavior target:** measured latency is reported and asserted under a budget
  on synthetic sequences; deterministic across runs.
- **Do NOT:** claim hardware latency numbers; measure only the engine path.

## 4. Architectural Constraints (HARD REQUIREMENTS)

- **Realtime path is lock-free:** no mutex, no `malloc`/`new`, no syscall, no file
  I/O in the audio/dispatch callback. Cross-thread state via `std::atomic` /
  lock-free queues only.
- **No engine regressions:** the 29 existing test binaries / 430 assertions stay
  green; existing public behaviour unchanged unless a test codifies the change.
- **Deterministic:** identical control sequences produce identical event streams
  (replayable). All control intent is quantized through the sequencer/clock.
- **Layered on existing components:** reuse `RealtimeClock`, `SectionSequencer`,
  `MidiScheduler`, `PanicHandler`, `Ntt`, `groove`; do not fork them.
- **Vendor-agnostic engine:** Korg/Yamaha semantics live in adapters/docs, never
  hardcoded in `src/engine/`.

## 5. FORBIDDEN

- ❌ No claim of hardware parity or KORG/Yamaha compatibility.
- ❌ No DSP / audio synthesis.
- ❌ No audio rendering.
- ❌ No UI coupling (engine must not depend on any UI).
- ❌ No bypassing the scheduler (all sounding events go through `MidiScheduler`).
- ❌ No allocation/locks/syscalls on the realtime path.

## 6. Suggested Structure (from spec, expanded in §11)

`src/engine/performance/` (sync, fill, variation, split, realtime state machine,
performer panic), `src/engine/control/` (control surface + events),
`src/engine/chord/` (scan-mode interface + detectors), with `tests/realtime/` and
`fixtures/realtime/` for synthetic control sequences.

## 7. Testing Requirements

- **Rapid spam:** flood fill/variation/sync triggers → collapses correctly, no
  double-fire, no stuck notes.
- **Panic recovery:** panic from every state → zero hanging notes, clean re-arm.
- **Race conditions:** concurrent control thread vs dispatch thread (atomics /
  lock-free queue) → no torn reads, deterministic outcome (TSan-friendly design).
- **Latency benchmarks:** control→effect latency measured + asserted under budget.
- **Deterministic replay:** the same recorded control sequence yields a
  byte-identical dispatched stream across runs.

## 8. Deliverables

- `src/engine/performance/`, `src/engine/control/`, `src/engine/chord/` modules.
- `tests/realtime/` synthetic-fixture test binaries (CI-safe, no hardware).
- `fixtures/realtime/synthetic_*.csv` control sequences.
- A latency/behaviour report (JSON + Markdown) that **MUST** include
  `deterministic: true` and `hardware_validated: false`.
- `GATE_12_HANDOFF.md` + `HANDOFF_MASTER.md` update.

## 9. Success Criteria

- The engine **feels realtime-safe** (observable via logs/state, no blocking).
- **Transitions deterministic** (state-machine replay).
- **Fill timing stable** (jitter within budget).
- **Panic recovery reliable** (rapid recovery from any state).
- **Latency measurable** (benchmark in CI).
- **No regression** in the 29 existing binaries.
- **CI green.**

---

## 10. Module design per scope item

### 10.1 SyncController
- **I/O:** in `ControlEvent{SyncArm, Stop}` + chord-present signal; out: arms/fires
  `StylePlayer::start()` / `stop()` at the quantize boundary.
- **State:** `Disarmed → Armed → Running → Stopping`.
- **RT-safety:** `std::atomic<State>` + atomic armed flag; decision made in the
  dispatch tick (no alloc).
- **Test:** arm → first chord at tick T → start lands on next boundary; stop
  clears to Disarmed; re-arm idempotent.
- **Interacts:** drives `SectionSequencer`/`StylePlayer`; never starts audio.

### 10.2 FillScheduler
- **I/O:** in `ControlEvent::Fill{target}`; out: queued fill index to the sequencer.
- **State:** `Idle → Queued → Firing → Returning`.
- **RT-safety:** atomic pending-fill slot; rapid presses overwrite (collapse), no
  queue growth.
- **Test:** spam 100 fill events in one bar → exactly one fill fires on the
  boundary; auto-return to owning main; no stuck notes.
- **Interacts:** `SectionSequencer::queueFill()` + boundary logic.

### 10.3 VariationManager
- **I/O:** in `ControlEvent::Variation{A..D}` (+ intro/ending); out: queued section.
- **State:** current variation + pending target; policy `NextBar|Immediate`.
- **RT-safety:** atomic pending target; last write wins.
- **Test:** queue B then C before boundary → C applies once; immediate policy
  switches mid-bar with a flush (no stuck notes).
- **Interacts:** `SectionSequencer::queueSection()`; reuses Gate 10B quantize.

### 10.4 Chord scan framework + 4 detectors
- **I/O:** in held lower-zone notes (note set); out: `Chord`.
- **State:** stateless per scan (pure function of the held set + mode).
- **RT-safety:** fixed-size note buffer on stack; no alloc.
- **Test:** per detector, fixed key sets → expected `Chord` (Fingered C-E-G→Cmaj;
  Single-Finger C→Cmaj, C+♭7 key→C7 per documented rule; on-bass slash; AI
  placeholder → documented fallback == Fingered).
- **Interacts:** feeds `StylePlayer::setChord()`; pairs with `Ntt`.

### 10.5 SplitRouter
- **I/O:** in raw note events + split point + manual-bass flag; out: routed
  (lowerZone→chord, upperZone→manual/melody, lowest→bass override).
- **State:** split point + flags (atomic).
- **RT-safety:** pure routing, stack-only.
- **Test:** notes below/above split route correctly; manual bass overrides NTT
  bass when enabled.
- **Interacts:** front of chord-scan + bass path.

### 10.6 ControlSurface abstraction
- **I/O:** `ControlEvent` enum + payload; `IControlSurface` produces events; the
  performer layer consumes them.
- **State:** none (transport).
- **RT-safety:** lock-free SPSC ring for control→dispatch hand-off.
- **Test:** a synthetic surface emits a scripted sequence; the layer reacts
  identically regardless of source.
- **Interacts:** the single entry point for all performer actions.

### 10.7 PerformerPanic (extends Gate 9 PanicHandler)
- **I/O:** in `ControlEvent::Panic`; out: all-notes-off + flush + re-arm.
- **State:** `Active → Panicking → Recovered`.
- **RT-safety:** reuses `PanicHandler` lock-free flush; no alloc.
- **Test:** panic from Running/Fill/Transition → zero hanging notes; re-arm works
  within budget; rapid panic spam safe.
- **Interacts:** wraps `PanicHandler`; resets Sync/Fill/Variation state.

### 10.8 Latency budget + reporter
- **I/O:** in control-event timestamps + dispatched-event timestamps; out: latency
  stats + pass/fail vs budget.
- **State:** none (analyzer).
- **RT-safety:** measurement is offline/aggregation (not on RT path).
- **Test:** synthetic control→effect pairs → asserted latency under budget;
  deterministic across runs.
- **Interacts:** extends the Gate 11 reporter; emits `deterministic:true` +
  `hardware_validated:false`.

## 11. File layout

```
src/engine/performance/
├── sync_controller.{h,cpp}
├── fill_scheduler.{h,cpp}
├── variation_manager.{h,cpp}
├── split_router.{h,cpp}
├── realtime_state_machine.{h,cpp}
└── performer_panic.{h,cpp}
src/engine/control/
├── control_surface.{h,cpp}      # abstract interface
└── control_events.{h,cpp}       # button/event types
src/engine/chord/
├── chord_scan_mode.{h,cpp}      # mode interface
├── fingered_detector.{h,cpp}
├── fingered_on_bass_detector.{h,cpp}
├── single_finger_detector.{h,cpp}
└── ai_fingered_placeholder.{h,cpp}
tests/realtime/
├── test_sync_controller.cpp
├── test_fill_scheduler.cpp
├── test_variation_manager.cpp
├── test_chord_scan.cpp
├── test_split_router.cpp
├── test_realtime_state_machine.cpp
├── test_performer_panic.cpp
└── test_latency_budget.cpp
fixtures/realtime/
└── synthetic_<scenario>.csv     # rapid spam, race, recovery, …
```
> CMake: `tests/realtime/*.cpp` joins the per-file CTest pattern (or an
> `add_subdirectory`) so the new tests run under `scripts/run_tests.sh` + CI with
> no workflow change. New engine sources join the `arranger-engine` globs
> (`src/engine/performance|control|chord/*.cpp`).

## 12. Task breakdown (G12-TASK-A → J)

| Task | Commit | Content |
|---|---|---|
| A | `G12-TASK-A` | `realtime_state_machine` foundation (states/transitions, atomic, replay hook) + tests |
| B | `G12-TASK-B` | `sync_controller` (sync start/stop, quantized) + tests |
| C | `G12-TASK-C` | `fill_scheduler` (queue/collapse/fire/return) + tests |
| D | `G12-TASK-D` | `variation_manager` (A–D, quantize, last-write-wins) + tests |
| E | `G12-TASK-E` | `chord/` scan framework + Fingered / on-Bass / Single-Finger / AI placeholder + tests |
| F | `G12-TASK-F` | `split_router` (split/lower/manual bass) + tests |
| G | `G12-TASK-G` | `control/` surface abstraction + events + lock-free SPSC + tests |
| H | `G12-TASK-H` | `performer_panic` (extends G9 panic; recovery/re-arm) + rapid-spam tests |
| I | `G12-TASK-I` | latency budget benchmarks + reporter (extends G11; `deterministic=true`, `hardware_validated=false`) + sample report |
| J | `G12-TASK-J` | `GATE_12_HANDOFF.md` + `HANDOFF_MASTER.md` update |

One commit per task; full suite green after each (29 baseline + N new).

## 13. Acceptance criteria (checklist)

- ☐ Engine feels realtime-safe (observable via state/logs; no blocking on RT path).
- ☐ Transitions deterministic (state-machine replay test passes).
- ☐ Fill timing stable (jitter within budget; reuses G11 jitter + new test).
- ☐ Panic recovery reliable (rapid recovery from any state; zero hanging notes).
- ☐ Latency measurable (benchmark asserted in CI).
- ☐ No regression in the 29 existing binaries / 430 assertions.
- ☐ CI green.
- ☐ Every report emits `deterministic: true` + `hardware_validated: false`.

## 14. Effort estimate

- **New source:** ~30 files (≈18 headers/impls across performance/control/chord +
  CMake glue).
- **New tests:** ~10–15 CTest binaries + synthetic control fixtures.
- **Complexity:** medium–high — concurrency (control vs dispatch threads),
  state-machine determinism, lock-free SPSC. No DSP, no UI.
- **Deps:** none new (atomics/STL only; hand-rolled lock-free ring).
- **Risk:** medium — RT-safety + determinism are the hard parts; mitigated by
  synthetic replay tests and reuse of proven Gate 9/10 components.

## 15. Status rules (baked in)

- All Gate 12 reports emit **`deterministic: true`** and
  **`hardware_validated: false`** (hard-coded, like Gate 11).
- No report or doc sets `claims_modified: true`; the Claims Policy is unchanged.
- **Forbidden phrases** (must not appear as claims): "compatible with Korg/Yamaha",
  "hardware validated", "tested on PA700/PA1000", "real-device parity".
- Software/realtime-harness PASS is allowed ("deterministic on synthetic control
  sequences"); hardware parity remains DEFERRED_BY_PTH.

---

> When approved, implement per §12 — one commit per task, tests green after each,
> PR at the end. No hardware/KORG compatibility claims; reports stay
> `deterministic:true` + `hardware_validated:false`.
