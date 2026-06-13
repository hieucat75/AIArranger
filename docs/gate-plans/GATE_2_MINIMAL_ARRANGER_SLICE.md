# GATE 2 — Minimal Arranger Vertical Slice Plan

> **Project:** AI Arranger / Style Maker  
> **Status:** 🔨 IN PROGRESS  
> **Date:** 2026-06-13  
> **Prerequisite:** Gate 1 ✅ (timing spike passed)

## Objective

Play a minimal internal style (Intro → Main → Fill → Ending) with bar-boundary section switching and chord input, driven by real CoreAudio callback timing.

## Scope

- CoreAudio realtime audio unit hook
- mach_absolute_time() clock backend
- Minimal style playback engine (4-section)
- Bar-boundary section switching
- Simulated chord input
- Multi-track MIDI playback
- Start/stop/panic from engine

## Non-Goals

- UI of any kind (no SwiftUI/UIKit)
- Internal sound synthesis
- Yamaha/Korg importer
- File-based style loading (hardcoded for now)
- Live MIDI input (simulated chords only)
- AUv3 integration

## Architecture Rules

- Runtime only understands UASF types
- No Yamaha/Korg logic in runtime
- No malloc/mutex/fileIO in realtime callback
- No silent MegaVoice→GM mapping (MegaVoice = deferred with warning)
- mach_absolute_time() for clock (no std::chrono in callback)

## Deliverables

| # | Deliverable | File | Status |
|---|-------------|------|--------|
| D1 | CoreAudio driver | `src/engine/realtime/coreaudio_driver.h/.cpp` | |
| D2 | mach_absolute_time clock | `src/engine/realtime/clock.h/.cpp` (updated) | |
| D3 | Style player | `src/engine/arranger/style_player.h/.cpp` | |
| D4 | Section sequencer | `src/engine/arranger/section_sequencer.h/.cpp` | |
| D5 | Chord input (simulated) | `src/engine/arranger/chord_input.h/.cpp` | |
| D6 | Gate 2 benchmark | updated `timing_benchmark.cpp` | |
| D7 | Unit tests | `tests/engine/test_*.cpp` | |
| D8 | Timing report (real clock) | `docs/evidence/gate-2/` | |

## Exit Criteria

- [ ] App/engine plays a minimal internal style
- [ ] Section switching on correct bar/beat boundary
- [ ] Chord change affects playback deterministically
- [ ] Panic works during playback (all notes stop within 1 callback)
- [ ] Timing measured on mach_absolute_time() / CoreAudio path
- [ ] All tests pass
- [ ] Evidence report created
