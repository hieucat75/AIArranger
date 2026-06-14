# Gate 14C — Handoff (CoreMIDI Output Wiring)

> Branch `gate-14c-coremidi-output` off `main` (`6b95d32`).
> **Status: ✅ Gate 14C Engine-side PASS (headless) / GUI builds locally / DAW
> verification PENDING PTH.**
> 61 test binaries, 720 assertions, 0 failures (headless). JUCE app builds with
> JUCE 8.0.4 (local). Engine MIDI now routes to a selectable CoreMIDI output.
> No arranger logic in the MIDI layer; lock-free playback path. Reports emit
> `deterministic: true` + `hardware_validated: false`. No KORG/Yamaha claim.

---

## 1. Scope (5 items) — delivery

| # | Item | Where | Headless test |
|---|------|-------|:--:|
| 1 | MIDI Output Device Enumeration | `src/midi/midi_output_manager` (+ provider) | ✅ |
| 2 | Engine MIDI Event Routing | `src/control/{midi_output_bridge,engine_midi_route}` | ✅ |
| 3 | IAC Driver Verification | `GATE_14C_MANUAL_DAW_VERIFICATION.md` | manual (PTH) |
| 4 | MIDI Output Monitor Panel | `src/ui/midi/midi_output_viewmodel` + `apps/macos/.../MidiOutputPanel` | ✅ (VM) |
| 5 | Failure Handling | manager state machine + provider no-op | ✅ |

## 2. Commits

| Hash | Title |
|---|---|
| `9d7cefc` | A: MIDI output provider interface + CoreMIDI impl |
| `d2e1d22` | B: MIDI output manager + connection state machine |
| `a4d2ad7` | C: engine→MIDI output bridge (lock-free SPSC) |
| `d12c4fb` | D: wire engine scheduler → bridge → provider |
| `d7a8c8d` | E: MIDI output ViewModel (UI-agnostic) |
| `54b9c81` | F: MidiOutputPanel (JUCE) + route driver via provider/bridge/manager |
| `5c87ea8` | G: failure handling tests (disconnect/reconnect during playback) |

(+ Task H manual doc + this handoff = Task I.)

## 3. Architecture

```
StylePlayer.scheduler ──output cb──▶ MidiOutputBridge (SPSC, lock-free)
                                          │ pumpEngineOutput (dispatch side)
                                          ▼
                                   IMidiOutputProvider ──▶ CoreMidiOutputProvider
                                          ▲                 (wraps Gate 9 CoreMidiOut)
                       MidiOutputManager ─┘ (enumerate/select/state, by name)
   UI: MidiOutputPanel ─▶ MidiOutputViewModel ─selectSink─▶ MidiOutputManager
```

- **No CoreMIDI in the engine path** — the scheduler callback only pushes onto the
  bridge; CoreMIDI lives solely in `CoreMidiOutputProvider` (wrapping the Gate 9
  `CoreMidiOut`, no behaviour change).
- **No arranger logic in the MIDI layer.** **No UI→CoreMIDI bypass** — UI selects
  via the ViewModel sink → manager. **Lock-free playback path** (SPSC).
- Provider abstraction → fully CI-testable with a synthetic provider.

## 4. Test evidence

| Test (new, headless) | Assertions |
|---|--:|
| test_midi_output_provider | 10 |
| test_midi_output_manager | 11 |
| test_midi_output_bridge | 8 |
| test_engine_midi_route | 6 |
| test_midi_output_viewmodel | 11 |
| test_midi_output_failure | 13 |
| **Gate 14C new total** | **59** |

- Full suite: **61 binaries, 720 assertions, 0 failures** (was 55 / 661).
- Covers: no-device state, reconnect after disconnect, endpoint switch, event
  dispatch count, concurrent flood in-order, engine survives disconnect.
- All synthetic (no CoreMIDI in CI). JUCE app validated by a **local** build
  (JUCE 8.0.4: 100% built, no errors).

## 5. Decisions worth noting

1. **Reuse Gate 9 `CoreMidiOut` unchanged**, wrapped behind `IMidiOutputProvider`
   — no duplicate CoreMIDI code, no behaviour change.
2. **Two-stage hand-off** (engine→bridge, bridge→provider) keeps the playback path
   free of CoreMIDI and lets the provider be swapped for a fake in CI.
3. **Selection remembered by name** → hot-plug reconnects to the same device
   (mirrors `CoreMidiOut`'s re-resolution).
4. **Manager state machine** (NoDevice/Selected/Connected/Disconnected/
   Reconnecting) is informational; sending no-ops safely when not connected, so
   the engine keeps running with no device.
5. **GUI not CI-built** (CI-optional); validated locally. DAW/IAC audibility is a
   manual gate — `hardware_validated: false` (this is audible routing, not
   external-hardware validation).

## 6. Residual risks / status rules

- **Software/headless PASS.** Hardware parity + KORG compatibility claims
  **FORBIDDEN** until committed real-device evidence (Claims Policy).
- **DAW/IAC audibility PENDING PTH** (`GATE_14C_MANUAL_DAW_VERIFICATION.md`).
- The MIDI monitor panel's per-event last-message/sent-count is fed in the
  headless VM tests; the GUI mirrors device list + connection state each tick
  (per-event UI feed via a dispatch tap is a small follow-up).
- Engine driver still uses the ~1ms prototype timer (carried from Gate 14).

## 7. Merge status

**MERGE_ALLOWED: NO** — pending PTH review (Gate discipline). Branch pushed; PR to
be opened. CI expected green (headless; GUI not built in CI).
