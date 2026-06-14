# Gate 13 — Handoff (StylePlayer End-to-End Performer Integration)

> Branch `gate-13-styleplayer-performer-integration` off `main` (`dc69980`).
> **Status: ✅ Gate 13 Engine PASS (deterministic, synthetic).**
> 46 test binaries, 576 assertions, 0 failures.
> Integration/wiring only — **no DSP, no audio, no UI, no hardware**. Reports emit
> `deterministic: true` + `hardware_validated: false` (hard-coded). No KORG/Yamaha
> compatibility claimed. See `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. What this gate did

Wired the Gate 12 performer modules (previously decision-only, no consumer) INTO
the real `StylePlayer` playback path via a stateless adapter
(`src/engine/integration/performer_adapter`). Control events and chord input now
drive actual section changes (bar-quantized), NTT transpose, and panic/recovery.

## 2. Commits

| Hash | Title |
|---|---|
| `452b72a` | A: performer integration adapter skeleton |
| `0d9c54a` | B: SyncController ↔ StylePlayer E2E |
| `684cb8b` | C: FillScheduler ↔ sequencer E2E (bar-boundary) |
| `017e211` | D: VariationManager ↔ sequencer E2E (bar-boundary) |
| `542c52e` | E: ChordScanMode ↔ StylePlayer E2E |
| `fe8a659` | F: SplitRouter ↔ StylePlayer E2E |
| `d74f016` | G: control surface E2E (keystroke → ControlEvent → adapter) |
| `3158f7f` | H: end-to-end control→dispatch latency benchmark |

(+ this handoff commit = Task I.)

## 3. Architecture

`PerformerAdapter` (in `arranger-engine`) owns the Gate 12 modules (control
intent) and holds **no playback state** — `StylePlayer`/`SectionSequencer` remain
the playback/bar authority. Flow:

- Control thread → `postEvent()` → lock-free SPSC `ControlEventQueue`.
- `adapter.tick()` drains the queue, forwards collapsed intent to existing
  `StylePlayer` hooks (`start/stop/fill/switchSection/ending/panic`), then calls
  `player.tick()`. Fill/variation bar-boundary resolution is the **sequencer's**
  existing quantize (no second authority, no double-delay).
- Notes → `SplitRouter` (zone) → `IChordScanMode` → `StylePlayer::setChord()` →
  existing NTT transpose. Sync-start arms, first chord fires `start()` once.
- Panic → `player.panic()` (single PanicHandler) + reset performer module state.

**No engine-core behaviour change** — only the adapter is new; `StylePlayer`,
`Scheduler`, `Sequencer` are used through their existing public hooks.

## 4. Test evidence

| Test (new) | Assertions |
|---|--:|
| test_performer_adapter | 7 |
| test_e2e_sync | 5 |
| test_e2e_fill | 2 |
| test_e2e_variation | 4 |
| test_e2e_chord | 8 |
| test_e2e_split | 5 |
| test_e2e_control_surface | 9 |
| test_e2e_latency | 9 |
| **Gate 13 new total** | **49** |

- Full suite: **46 binaries, 576 assertions, 0 failures** (was 38 / 527).
- Covers: control→playback (start/stop), fill at bar boundary (+ spam collapse),
  variation switch at boundary (last-wins, no mid-bar cut), chord detection→
  transpose across 4 modes, split/manual-bass routing, keystroke E2E, panic
  recovery + re-arm, control→dispatch latency.
- All synthetic; no CoreMIDI/network in any test. Baseline 38 binaries unchanged.

## 5. Sample report (committed)

`tests/realtime/sample/e2e_latency_report.{json,md}` — excerpt:
```json
{ "deterministic": true, "hardware_validated": false,
  "scenario": "e2e_control_to_dispatch", "all_within_budget": true,
  "metrics": [ { "name": "control_to_section_switch", "max_ms": 1899.0, "budget_ms": 2050.0 } ] }
```

## 6. Decisions worth noting

1. **Sequencer stays the single bar-boundary authority.** The adapter forwards
   collapsed intent to existing `StylePlayer` hooks rather than re-implementing
   quantize — avoids duplicate state and a double-bar delay; no core change.
2. **Adapter is stateless re: playback** — modules own control intent, player
   owns playback state. Clean ownership.
3. **Manual bass asserted on the adapter's routed chord** (`lastChord`), because
   `SectionSequencer` persists only chord type+root (bass is dropped). Using
   `chord.bass` in dispatch is a future NTT enhancement — an engine-core change
   deliberately NOT made in Gate 13.
4. **CLI not rewired in CI.** The shared `key_bindings.h` map + headless E2E test
   prove the keystroke→adapter→player path; swapping the live CoreMIDI
   `korg-playback` loop to the adapter is a thin follow-up kept out of CI.
5. **Reports hard-code `deterministic:true` + `hardware_validated:false`** (reuse
   Gate 12 reporter) — cannot accidentally claim hardware parity.

## 7. Residual risks / status rules

- **Software/E2E PASS allowed** ("deterministic on synthetic control sequences").
  **Hardware parity + KORG compatibility claims FORBIDDEN** until committed
  real-device evidence (Claims Policy). Reports stay `hardware_validated:false`.
- **`SectionSequencer` drops chord bass** — manual-bass/slash voicing won't reach
  dispatch until a (future) NTT/sequencer enhancement carries `chord.bass`.
- **Live `korg-playback` CLI** still uses its own key handler; full adapter swap
  is pending.
- Latency is **logical-step / quantize latency**, not device wall-clock.

## 8. Merge status

**MERGE_ALLOWED: NO** — pending PTH review (Gate discipline). Branch pushed; PR to
be opened. CI expected green (synthetic-only).
