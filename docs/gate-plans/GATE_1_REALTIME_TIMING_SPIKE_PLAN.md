# GATE 1 — Realtime Timing Spike Plan

> **Project:** AI Arranger / Style Maker  
> **Status:** 🔨 IN PROGRESS  
> **Owner:** OpenClaw (Coordinator) + Codex (Implementation)  
> **Date:** 2026-06-13

## Objective

Prove that the engine can deliver stable MIDI timing with measurable jitter < 1ms P99 in standalone mode.

## Scope

- Realtime clock (sample-position-based)
- MIDI scheduler (lock-free event queue)
- Panic / All-Notes-Off handler
- Timing benchmark tool
- Unit tests for all modules

## Non-Goals

- Audio playback
- AUv3 / CoreAudio integration
- JUCE dependency (postponed to Gate 2)
- UI of any kind
- Yamaha/Korg import
- Internal sound synthesis

## Architecture Rules

- No malloc in realtime path
- No mutex in realtime path
- No file IO in realtime callback
- No Objective-C/Swift/ARC in realtime path
- Vendor-neutral runtime: no Yamaha/Korg logic
- No silent MegaVoice→GM mapping (deferred with warning)

## Deliverables

| # | Deliverable | File | Status |
|---|-------------|------|--------|
| D1 | Real-time clock | `src/engine/realtime/clock.h/.cpp` | |
| D2 | MIDI scheduler | `src/engine/midi/scheduler.h/.cpp` | |
| D3 | Panic handler | `src/engine/midi/panic.h/.cpp` | |
| D4 | Timing benchmark | `src/engine/realtime/timing_benchmark.h/.cpp` | |
| D5 | UASF type definitions | `src/engine/uasf/types.h` | |
| D6 | Unit tests | `tests/engine/test_*.cpp` | |
| D7 | Timing report | `docs/evidence/gate-1/timing-report.md` | |
| D8 | Architecture review | `docs/evidence/gate-1/architecture-review.md` | |
| D9 | Build (CMake) | `CMakeLists.txt` | |

## Exit Criteria

- [ ] Build + all tests pass
- [ ] Scheduler simulation works (non-blocking, tick-accurate)
- [ ] Timing benchmark generated with P99 jitter measurement
- [ ] Panic test: all active notes silenced within 1ms
- [ ] Architecture review completed
- [ ] No vendor-specific runtime contamination (Yamaha/Korg/Roland logic is ZERO in runtime)
- [ ] No silent MegaVoice→GM mapping

## RACI

| Task | OpenClaw | Codex | Claude | Gemini |
|------|----------|-------|--------|--------|
| Clock implementation | A | R | C | I |
| MIDI scheduler | I | R | C | I |
| Panic handler | I | R | C | I |
| Timing benchmark | I | R | A | C |
| Unit tests | I | R | I | C |
| Architecture review | A | C | R | I |
| Docs/evidence | R | C | I | I |

## Risks

| # | Risk | Likelihood | Impact | Mitigation |
|---|------|------------|--------|------------|
| R1 | std::chrono not accurate enough for < 1ms | Low | High | Use mach_absolute_time() on macOS/iOS |
| R2 | Lock-free queue has undetected race | Low | Critical | Bounded SPSC queue with memory barrier tests |
| R3 | Panic doesn't clear pending events | Low | Medium | Two-phase: flush queue + send NoteOff |

## Decision: PENDING (after all exit criteria met)
