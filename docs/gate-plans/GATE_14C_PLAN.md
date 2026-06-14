# Gate 14C — Plan: CoreMIDI Output Wiring (macOS)

> **PROPOSAL ONLY — no code in this document.** Makes the Gate 14 macOS app
> actually *audible* by routing the engine's MIDI out through CoreMIDI to an IAC
> bus / DAW / external synth — without changing the arranger engine architecture.
>
> Authoring note: composed from the provided Gate 14C scope plus the codebase.
> Every report emits `deterministic: true` + `hardware_validated: false`. No
> KORG/Yamaha compatibility claim. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Context

Gate 14B confirmed the app launches, the UI is responsive, transport/chord are
functional, the realtime engine runs, no crash, deterministic. **However:**

- No MIDI output device is selectable.
- No CoreMIDI routing from the engine to an external destination.
- Logic / Reaper do not receive the arranger's MIDI.
- The app is visually playable but **NOT externally audible**.

## 2. Goal

Enable **AI Arranger → CoreMIDI OUT → IAC Driver / DAW / external synth →
audible playback**, without changing the arranger engine architecture (engine
still owns timing/scheduling; UI still sends intent only).

## 3. Scope (5 items)

1. **MIDI Output Device Enumeration** — list CoreMIDI destinations, select one,
   hot-plug aware.
2. **Engine MIDI Event Routing** — engine scheduler output → CoreMIDI OUT via a
   lock-free hand-off (reuse the Gate 9 `CoreMidiOut`).
3. **IAC Driver Verification** — manual checklist (Logic / Reaper / MIDI Monitor).
4. **MIDI Output Monitor Panel** — telemetry: selected output, sent-event count,
   last MIDI message, connection status.
5. **Failure Handling** — no device / disconnect / reconnect, engine keeps
   scheduling, MIDI dispatch no-ops safely until connected.

## 4. Architectural Constraints (HARD RULES)

- **NO arranger logic inside the MIDI layer.**
- **NO direct UI → CoreMIDI bypass** (UI selects a device via a ViewModel; the
  engine thread does the sending).
- **NO engine-timing ownership in the UI.**
- **NO blocking locks on the playback path** (lock-free SPSC only).
- **Deterministic scheduling preserved.**

## 5. Forbidden

- ❌ No internal synth. ❌ No AU/VST hosting. ❌ No audio rendering. ❌ No
  waveform output. ❌ No sample playback.
- ❌ No claims-policy change. ❌ No KORG/Yamaha compatibility claim. ❌ No
  hardware parity claim.

## 6. Suggested Structure

`src/midi/` (output manager + CoreMIDI provider behind an interface),
`src/control/` (engine→MIDI output bridge), `src/ui/midi/` (output ViewModel),
`apps/macos/Source/` (MidiOutputPanel), `tests/midi/` (synthetic), plus the
manual DAW checklist doc. Detailed layout in §11.

## 7. Testing Requirements

- **Synthetic (CI-safe, no CoreMIDI):** enumeration via a fake provider; hot-plug
  add/remove; output-bridge event flood (race-free, bounded); failure state
  machine (no device / disconnect / reconnect); ViewModel state.
- **Manual (PTH):** Logic / Reaper / MIDI Monitor receive NoteOn/Off; playback
  survives device reconnect; no UI freeze.

## 8. Deliverables

- MIDI output manager + CoreMIDI provider (interface-abstracted).
- Engine→MIDI output bridge (lock-free).
- MidiOutputViewModel + MidiOutputPanel (JUCE).
- `GATE_14C_MANUAL_DAW_VERIFICATION.md` (PTH).
- Synthetic CI tests; full headless suite green.
- Reports MUST include `deterministic: true` + `hardware_validated: false`.

## 9. Success Criteria

- A DAW / MIDI Monitor receives the arranger's NoteOn/Off via a selected
  CoreMIDI destination (manual).
- Output device is selectable + hot-plug aware.
- Playback survives device disconnect/reconnect (no crash, engine keeps running).
- No UI freeze; no timing regression (existing tests green).
- Deterministic behavior preserved.

---

## 10. Module design

### 10.1 MIDI Output Device Enumeration
`MidiOutputManager` enumerates destinations via an `IMidiOutputProvider`
(CoreMIDI impl in `coremidi/`). Hot-plug via `MIDINotifyProc` → atomic refresh of
a cached list; UI binds a dropdown to `MidiOutputViewModel`. Mirrors the Gate 14
`MidiDeviceManager` pattern (provider-abstracted → CI-testable with a fake).

### 10.2 Engine MIDI Event Routing
Reuse the Gate 9 `CoreMidiOut` for the actual send. The engine scheduler output
callback pushes events onto a lock-free SPSC (`MidiOutputBridge`); a MIDI
dispatch consumer drains and sends. The engine never references CoreMIDI directly
beyond the existing `CoreMidiOut`; the bridge keeps the playback path non-blocking.

### 10.3 IAC Driver Verification
Manual checklist `GATE_14C_MANUAL_DAW_VERIFICATION.md` (Audio MIDI Setup → enable
IAC Bus 1 → select in app → play → confirm in Logic/Reaper/MIDI Monitor), same
pattern as Gate 9 / Gate 14B manual docs.

### 10.4 MIDI Output Monitor Panel
JUCE panel, **telemetry only**: selected output name, sent-event counter, last
MIDI message (ring buffer 16), connection status (NONE / SELECTED / CONNECTED /
DISCONNECTED). Reads `MidiOutputViewModel`; no engine calls.

### 10.5 Failure Handling
State machine `NO_DEVICE → SELECTED → CONNECTED → DISCONNECTED → (reconnecting)`.
Empty-state safe: the engine keeps scheduling; MIDI dispatch is a no-op until a
device is connected. `MIDINotify` triggers a reconnect attempt by id.

## 11. File layout

```
src/midi/
├── midi_output_manager.{h,cpp}        # enumeration + hot-plug + state
└── coremidi/
    └── coremidi_output_provider.{h,cpp}  # CoreMIDI impl behind IMidiOutputProvider
src/control/
└── midi_output_bridge.{h,cpp}         # engine -> MIDI dispatch (SPSC, lock-free)
src/ui/midi/
└── midi_output_viewmodel.{h,cpp}      # UI-agnostic VM
apps/macos/Source/
└── MidiOutputPanel.{h,cpp}            # JUCE view (dropdown + telemetry)
tests/midi/
├── test_midi_output_manager.cpp       # synthetic enumerate + hot-plug
├── test_midi_output_bridge.cpp        # event flood, race-free
├── test_midi_output_failure.cpp       # no-device / disconnect / reconnect
└── test_midi_output_viewmodel.cpp     # state machine
docs/gate-plans/
├── GATE_14C_HANDOFF.md
└── GATE_14C_MANUAL_DAW_VERIFICATION.md  # PTH fills in
```

## 12. Task breakdown (G14C-TASK-A → I)

| Task | Commit | Content |
|---|---|---|
| A | `G14C-TASK-A` | `IMidiOutputProvider` interface + CoreMIDI impl (reuse/wrap Gate 9 `CoreMidiOut`; no behaviour change). |
| B | `G14C-TASK-B` | `MidiOutputManager` — enumeration + hot-plug + select-by-id + state machine. Synthetic tests. |
| C | `G14C-TASK-C` | `MidiOutputBridge` — engine→MIDI SPSC, non-blocking, no alloc. Flood test. |
| D | `G14C-TASK-D` | Wire EngineDriver/session scheduler output → bridge → provider. Clean ownership; no direct CoreMIDI in engine logic. |
| E | `G14C-TASK-E` | `MidiOutputViewModel` — connection/count/last-msg observable. State-machine test. |
| F | `G14C-TASK-F` | `MidiOutputPanel` (JUCE) — dropdown + refresh + telemetry. (GUI, CI-optional.) |
| G | `G14C-TASK-G` | Failure-handling tests — disconnect during playback, reconnect, no-device no-op, no crash. |
| H | `G14C-TASK-H` | `GATE_14C_MANUAL_DAW_VERIFICATION.md` (Logic/Reaper/MIDI Monitor checklist). |
| I | `G14C-TASK-I` | `GATE_14C_HANDOFF.md` + `HANDOFF_MASTER.md`. |

One commit per task; headless suite green after each.

## 13. Dependency on Gate 14B (PR #21)

- Gate 14B bumps JUCE 7.0.12 → 8.0.4. Gate 14C **implementation** needs JUCE 8.0.4
  (for `MidiOutputPanel`).
- This **plan PR is docs-only** → no JUCE dependency, branches from `main`.
- **Implementation should branch after PR #21 merges** (avoids a duplicate JUCE
  bump + merge conflict). Alternatively branch off
  `gate-14b-macos-playable-verification` if parallel work is desired.

## 14. CI strategy

- All tests headless via a synthetic `IMidiOutputProvider` (no CoreMIDI calls in
  CI). The MIDI dispatch send is exercised through the interface, not a device.
- JUCE GUI (`MidiOutputPanel`) builds only with `BUILD_MACOS_APP=ON` (CI=OFF).
- Manual DAW verification = PTH local.

## 15. Acceptance criteria (checklist)

- ☐ Logic/Reaper receives arranger MIDI (manual).
- ☐ MIDI Monitor shows NoteOn/Off (manual).
- ☐ Playback survives device reconnect (auto test).
- ☐ No UI freeze (manual + programmatic).
- ☐ No timing regression (existing suite green).
- ☐ Deterministic preserved (all reports `deterministic: true`).
- ☐ Reports emit `hardware_validated: false`.

## 16. Effort estimate

- **New source:** ~10–12 files (provider/manager/bridge/viewmodel/panel + tests).
- **New tests:** ~4–5 headless binaries.
- **Complexity:** medium — CoreMIDI integration + hot-plug state machine + SPSC
  bridge; reuses Gate 9 `CoreMidiOut` and the Gate 14 provider-abstraction pattern.
- **Deps:** none new (JUCE 8.0.4 already from Gate 14B).

## 17. Status rules (baked in)

- Every report hard-codes **`deterministic: true`** + **`hardware_validated:
  false`**.
- Claims Policy unchanged; no KORG/Yamaha compatibility claim.
- IAC/DAW playback is *audible routing*, NOT external-hardware validation — Korg
  PA validation stays DEFERRED_BY_PTH.

## 18. Risks

- **CoreMIDI hot-plug timing race** → handle via `MIDINotifyProc` + atomic state,
  never block.
- **Gate 9 `CoreMidiOut` integration** → may need wrapping behind the new
  interface; **no behaviour change** to the existing class.
- **IAC Driver setup is a user-side prerequisite** (Audio MIDI Setup → enable IAC
  Bus 1) — documented in the manual checklist.

---

> When approved (and after PR #21 merges), implement per §12 — one commit per
> task, headless tests green after each, PR at the end. No hardware/KORG
> compatibility claims; reports stay `deterministic:true` + `hardware_validated:false`.
