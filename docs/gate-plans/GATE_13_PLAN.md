# Gate 13 — Plan: StylePlayer End-to-End Performer Integration

> **PROPOSAL ONLY — no code in this document.** Wires the Gate 12 performer
> modules (which currently emit intent with no real consumer) INTO the real
> `StylePlayer` playback path, end to end. Closes the Gate 12 residual:
> *"live MIDI front-end adapter + end-to-end StylePlayer wiring = future work."*
>
> Inherited constraints: deterministic, synthetic-only, lock-free RT path, no DSP,
> no UI framework, **no hardware/vendor compatibility claim**. Every report emits
> `deterministic: true` + `hardware_validated: false`. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Goal

Make the performer layer actually drive playback: a `ControlEvent` (start, fill,
variation, panic) and chord input flow through the Gate 12 modules and produce
real `StylePlayer` dispatch — section changes resolved on the bar boundary,
chords transposed through the existing NTT path, panic resetting the scheduler
and re-arming. Today:

- Gate 12 modules (`SyncController`, `FillScheduler`, `VariationManager`,
  chord detectors, `SplitRouter`, `ControlEventQueue`, `PerformerPanic`,
  latency reporter) are **decision-only units** with no real consumer.
- `StylePlayer` (Gate 2/9/10) already exposes `start/stop/fill/ending/
  switchSection/setChord/tick/panic` and a bar-boundary `SectionSequencer`.

Gate 13 connects the two with a thin adapter — **integration, not new logic**.

## 2. Out of scope

- No new performer features (only wiring existing Gate 12 modules).
- No change to core `MidiScheduler` / `SectionSequencer` behaviour.
- No UI framework; no new hardware adapter (CoreMIDI already exists from Gate 9).
- No vendor compatibility / hardware parity claim.

## 3. Architecture (integration layer)

- A new **adapter** consumes performer intent and calls `StylePlayer` hooks; it
  holds **no playback state** of its own.
- **Clean ownership / no duplicate state:** performer modules own control intent
  (armed? pending fill? pending variation?); `StylePlayer`/`SectionSequencer` own
  playback state (current section, clock). The adapter only translates.
- **Bar-boundary resolution:** `FillScheduler`/`VariationManager` pending intents
  are resolved at the next bar boundary by the existing sequencer logic (reuse the
  Gate 10B quantize path); the adapter polls them inside `StylePlayer::tick()`.
- **Chord flow:** raw lower-zone notes → `SplitRouter` (zone) → `IChordScanMode`
  detector → `Chord` → `StylePlayer::setChord()` → existing NTT transpose.
- **Control flow:** front-end (CLI keystroke) → `ControlEvent` → `ControlEventQueue`
  → adapter (drained on tick) → `StylePlayer` hooks.

```
keystroke/MIDI ─▶ ControlEvent ─▶ [ControlEventQueue SPSC]
                                          │ drained in StylePlayer::tick()
                                          ▼
                                   PerformerAdapter
                 ┌────────────────┼─────────────────┬───────────────┐
                 ▼                ▼                  ▼               ▼
            SyncController   FillScheduler   VariationManager   PerformerPanic
                 │                │ (bar boundary)   │ (bar boundary) │
                 ▼                ▼                  ▼               ▼
            StylePlayer.start  .fill/sequencer  .switchSection   .panic
   notes ─▶ SplitRouter ─▶ ChordScanMode ─▶ StylePlayer.setChord ─▶ NTT transpose ─▶ scheduler
```

## 4. Tasks (G13-TASK-A → I)

| Task | Commit | Content |
|---|---|---|
| A | `G13-TASK-A` | `src/engine/integration/performer_adapter.{h,cpp}` — stateless connector: drains a `ControlEventQueue`, routes to performer modules + `StylePlayer` hooks. Wiring skeleton + tests (no behaviour change to engine core). |
| B | `G13-TASK-B` | SyncController ↔ StylePlayer: `arm()` → deferred `start()`; chord-present → auto-start. Tests: arm→chord→playing once. |
| C | `G13-TASK-C` | FillScheduler ↔ sequencer: pending fill resolved at next bar boundary inside tick; fires `StylePlayer::fill()` once. Tests: queue+spam→one fill on boundary. |
| D | `G13-TASK-D` | VariationManager ↔ sequencer: pending variation resolved at boundary → `switchSection()`. Tests: last-wins, single switch, no duplicate. |
| E | `G13-TASK-E` | ChordScanMode ↔ StylePlayer: detector output → `setChord()`; verify existing transpose produces expected notes. Tests across the 4 modes. |
| F | `G13-TASK-F` | SplitRouter ↔ StylePlayer: zone routing + manual-bass applied before chord detection / bass dispatch. Tests: lower→chord, upper ignored, manual bass override. |
| G | `G13-TASK-G` | ControlSurface end-to-end: extend `korg-playback` CLI keystrokes → `ControlEvent` → adapter → StylePlayer. Headless synthetic test drives the full chain (no CoreMIDI in test). |
| H | `G13-TASK-H` | E2E latency benchmark: extend the Gate 11/12 reporter to measure ControlEvent→MIDI-dispatch-tick latency; `deterministic:true` + `hardware_validated:false`; sample report committed. |
| I | `G13-TASK-I` | `GATE_13_HANDOFF.md` + `HANDOFF_MASTER.md` update. |

One commit per task; full suite green after each (38 baseline + N new).

## 5. Acceptance criteria

- A `ControlEvent::Fill` → `StylePlayer` dispatches the fill section at the next
  real bar boundary (deterministic tick).
- A `ControlEvent::VariationB/C/D` → section switch on the boundary (once).
- Chord input through a scan-mode detector → `StylePlayer` transposes correctly
  via the existing NTT path.
- `PerformerPanic` → scheduler reset + zero hanging notes + safe re-arm (no
  restart).
- The 38 baseline test binaries / 527 assertions stay green; no engine-core
  behaviour change.
- New E2E integration tests: rapid control events, bar-boundary resolution, chord
  changes during play, panic during play.
- E2E latency report shape correct, carries `deterministic: true` +
  `hardware_validated: false`.

## 6. Status rules (baked in)

- All reports emit **`deterministic: true`** and **`hardware_validated: false`**
  (hard-coded constants).
- Claims Policy unchanged; no `claims_modified`.
- **Forbidden phrases** (must not appear as claims): "compatible with Korg/Yamaha",
  "hardware validated", "tested on PA700/PA1000", "real-device parity".
- Software/E2E PASS allowed ("deterministic on synthetic control sequences").

## 7. Architectural constraints (inherited, hard requirements)

- **NO allocation / locks / syscalls on the audio/dispatch path.** Cross-thread
  via `std::atomic` / the existing lock-free SPSC ring.
- **Deterministic scheduling only** — all control intent quantized through the
  sequencer/clock; identical control sequences replay to identical dispatch.
- **CI-safe synthetic tests only** — no CoreMIDI device calls, no network, in any
  test.
- **No hardware/network dependency.**
- **No engine-core behaviour change** — `StylePlayer`/`Scheduler`/`Sequencer`
  modified only to expose hooks already present; the adapter is the new seam.

## 8. Effort estimate

- **New source:** ~10–15 files (`src/engine/integration/` adapter + headers +
  CMake glue + E2E reporter extension).
- **New tests:** ~5–8 CTest binaries (per-task E2E scenarios) + synthetic control
  fixtures.
- **Complexity:** medium — integration/wiring, not new algorithms. The risk is
  preserving determinism + RT-safety through the tick-drain path and keeping
  ownership clean (no duplicate state). No DSP, no UI.
- **Deps:** none new.

---

> When approved, implement per §4 — one commit per task, tests green after each,
> PR at the end. No hardware/KORG compatibility claims; reports stay
> `deterministic:true` + `hardware_validated:false`.
