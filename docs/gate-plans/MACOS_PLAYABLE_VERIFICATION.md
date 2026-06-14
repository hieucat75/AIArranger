# Gate 14B — macOS Playable Verification

## Build
- **Date:** 2026-06-14
- **Tag baseline:** `v0.14.0-macos-shell` (Gate 14 merge `b9615aa`)
- **Dep fix:** JUCE 7.0.12 → 8.0.4 (`4e62843`) — SDK-26 compatibility
- **Build-config fix:** project languages C/OBJC/OBJCXX + bundle id (`9576398`)
- **BUILD_MACOS_APP=ON**, `CMAKE_BUILD_TYPE=Release`
- **Duration:** configure (incl. JUCE 8.0.4 fetch + juceaide) ≈ 85s; app build ≈ 64s
- **Binary:** `build/apps/macos/aiarranger-macos_artefacts/Release/AI Arranger.app/Contents/MacOS/AI Arranger`
- **Warnings:** only `juce::Font(...)` deprecation (JUCE 8 prefers `FontOptions`) in
  ChordPanel.cpp / DiagnosticsPanel.cpp — non-fatal; app builds + links + runs.

## Programmatic Verifications (session)

| Check | Status | Detail |
|---|---|---|
| App builds (JUCE 8.0.4) | ✅ | binary above; 0 errors, 2 deprecation warnings |
| App launches | ✅ | `open` ok; PID 23253 |
| Process alive ~20s | ✅ | no crash/freeze across polling |
| CPU usage | ✅ | avg ≈29%, peak ≈32% (steady) — from the 1ms prototype HighResolutionTimer engine driver, NOT a spin/freeze |
| Memory baseline | ✅ | RSS ≈100 MB, flat (100224→100256 KB over 10s) |
| File handles stable | ✅ | 35 at launch → 35 after 10s (no leak) |
| Clean termination | ✅ | `kill` → terminated cleanly |
| MIDI device enumeration | ✅ (mechanism) | CoreMIDI: 0 destinations / 0 sources on this machine (no IAC bus / no hardware enabled). Enumeration path works and returns cleanly; populated list to be verified by PTH with a device/IAC. |
| Initial screenshot | ⏳ PTH | session has no window-server/screen-capture access ("could not create image from display"). Capture locally — see Recording. |
| Post-10s screenshot | ⏳ PTH | same as above |
| Headless suite (BUILD_MACOS_APP=ON) | ✅ | 55 binaries, 661 assertions, 0 failures |

> Note: ~29% CPU is the prototype engine driver ticking every 1ms; a true
> CoreAudio/MIDI callback clock (replacing the timer) is a later refinement and
> would lower idle CPU.

## Manual Verifications (PTH — fill in after running locally)

Build + run:
```
cmake -S . -B build -DBUILD_MACOS_APP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target aiarranger-macos --parallel
open "build/apps/macos/aiarranger-macos_artefacts/Release/AI Arranger.app"
```

| # | Check | Pass/Fail | Notes |
|---|---|---|---|
| 1 | Start → engine running (diagnostics shows playing) | ☐ | |
| 2 | Stop → engine stops cleanly | ☐ | |
| 3 | Sync Start → arms, auto-starts on chord | ☐ | |
| 4 | Sync Stop → graceful stop | ☐ | |
| 5 | Fill → fill at next bar boundary | ☐ | |
| 6 | Var A/B/C/D → variation switch at bar boundary | ☐ | |
| 7 | Panic → all notes off + safe re-arm | ☐ | |
| 8 | Chord pad → 12 roots × maj/min/7 emit chord | ☐ | |
| 9 | MIDI device list populated (enable IAC / connect device) | ☐ | |
| 10 | Hot-plug: connect device → list updates | ☐ | |
| 11 | No UI freeze during 60s playback + button spam | ☐ | |
| 12 | Screen recording captured | ☐ | path |

## Recording (PTH)
- Tool: QuickTime Player (Cmd+5 → screen recording) or `screencapture -v out.mov`
- Duration: ≥30s
- Save to: `docs/gate-plans/gate-14b-artifacts/playback.mov`
- Also save stills to: `docs/gate-plans/gate-14b-artifacts/launch.png`, `post10s.png`

## Status
- **Software build:** PASS (JUCE 8.0.4, SDK 26)
- **Programmatic checks:** PASS (launch, liveness, memory/CPU/handles stable, MIDI
  enumeration mechanism, headless suite 55/661/0)
- **Manual checks:** PENDING (PTH fills in locally — interactive clicks, hot-plug,
  screen recording, screenshots)
- **hardware_validated: false** — this gate does NOT validate against external
  hardware; Korg PA validation remains DEFERRED_BY_PTH.
- **deterministic: true** — engine/headless layer.
- No KORG/Yamaha compatibility claim; claims policy unchanged.
