# Gate 14C — Manual DAW / IAC Verification (for PTH)

> **Manual gate — runs locally.** Confirms the AI Arranger app sends MIDI through
> CoreMIDI to a DAW / MIDI Monitor via the IAC Driver. This is **audible routing
> verification**, NOT external-hardware validation — Korg PA validation remains
> **DEFERRED_BY_PTH**. `deterministic: true`, `hardware_validated: false`.

## 0. Prerequisite — enable the IAC Driver
1. Open **Audio MIDI Setup** (`/Applications/Utilities`).
2. Window → **Show MIDI Studio** → double-click **IAC Driver**.
3. Tick **"Device is online"**; ensure **Bus 1** exists. Apply.

## 1. Build + launch
```
cmake -S . -B build -DBUILD_MACOS_APP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target aiarranger-macos --parallel
open "build/apps/macos/aiarranger-macos_artefacts/Release/AI Arranger.app"
```

## 2. Select the output device
- In the **MIDI Output** panel, open the dropdown → expect **IAC Driver Bus 1**
  (and any connected synths). Select it.
- Status should read **connected**.

## 3. Verify in a MIDI Monitor (fastest check)
- Open **MIDI Monitor** (snoize) or Audio MIDI Setup's MIDI window.
- In the app: press **Start**, play a **chord** on the chord pad.
- Confirm a stream of **NoteOn / NoteOff** (and CC/PC) on IAC Bus 1.

## 4. Verify in a DAW (audible)
- **Logic Pro:** new Software Instrument track; input = IAC Bus 1; arm/monitor;
  load any instrument.
- **REAPER:** Preferences → MIDI Devices → enable IAC Bus 1 input; add a track
  with a VSTi; record-monitor.
- In the app: Start + chord/variation/fill → **hear** the arranger play.

## 5. Checklist (fill in)

| # | Check | Pass/Fail | Notes |
|---|---|---|---|
| 1 | IAC Bus 1 appears in the output dropdown | ☐ | |
| 2 | Selecting it shows status = connected | ☐ | |
| 3 | MIDI Monitor shows NoteOn/NoteOff | ☐ | |
| 4 | Logic/REAPER receives + sounds the arranger | ☐ | |
| 5 | Chord changes transpose audibly | ☐ | |
| 6 | Fill / Variation A–D audible at bar boundary | ☐ | |
| 7 | Panic stops all notes (no stuck notes) | ☐ | |
| 8 | Disconnect IAC (toggle offline) → app keeps running, no crash | ☐ | |
| 9 | Re-enable IAC → output reconnects (status connected) | ☐ | |
| 10 | No UI freeze during 60s playback + control spam | ☐ | |
| 11 | Monitor panel: sent count increases, last message updates | ☐ | |

## 6. Recording (optional)
- MIDI Monitor capture or screen recording → `docs/gate-plans/gate-14b-artifacts/`.

## Status
- Software build + routing: PASS (programmatic, headless — see
  `GATE_14C_HANDOFF.md`).
- DAW/IAC audible verification: **PENDING (PTH fills in)**.
- **hardware_validated: false** · **deterministic: true** · no KORG/Yamaha
  compatibility claim.
