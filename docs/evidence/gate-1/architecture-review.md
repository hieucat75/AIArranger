# GATE 1 — Architecture Review

> **Date:** 2026-06-13  
> **Reviewer:** OpenClaw (Coordinator) + Claude (Architecture)  
> **Status:** ✅ PASS

## Scope Reviewed

| Module | Lines | Status | Notes |
|--------|-------|--------|-------|
| `src/engine/uasf/types.h` | 120 | ✅ Clean | ADR-013 articulation metadata included |
| `src/engine/realtime/clock.h/.cpp` | 150 | ✅ Clean | Atomic, lock-free, sample-position based |
| `src/engine/midi/scheduler.h/.cpp` | 105 | ✅ Clean | Lock-free SPSC queue, no allocations |
| `src/engine/midi/panic.h/.cpp` | 120 | ✅ Clean | Two-phase: flush + NoteOff, CA-sync-safe |

## Architecture Rules Compliance

| Rule | Status | Evidence |
|------|--------|----------|
| No malloc in realtime path | ✅ PASS | All containers are std::array (stack); no heap allocation |
| No mutex in realtime path | ✅ PASS | All synchronization via std::atomic (lock-free) |
| No file IO in realtime callback | ✅ PASS | IO only in benchmark report writer (non-realtime) |
| No ObjC/Swift/ARC in realtime path | ✅ PASS | Pure C++20 |
| No vendor-specific runtime logic | ✅ PASS | Zero Yamaha/Korg/Roland references in engine code |
| Lock-free SPSC queue | ✅ PASS | `LockFreeQueue<T>` with atomic head/tail |
| No silent MegaVoice→GM mapping | ✅ PASS | MegaVoice handling deferred; articulation metadata preserved |
| ADR-013 compliance | ✅ PASS | Fidelity flags, articulation profiles, PlaybackRecommendation |

## Design Decisions

### D1: Sample-position-based clock vs OS timer
- **Chosen:** Sample-position-based from accumulated sample count
- **Rationale:** Matches CoreAudio render callback semantics. OS timers have unpredictable jitter (1-15ms).
- **Risk:** Requires audio callback to be active. Acceptable — this is always the case for a realtime engine.

### D2: Lock-free SPSC queue vs MPMC
- **Chosen:** Single-producer, single-consumer lock-free queue
- **Rationale:** Arranger engine → MIDI out is inherently SPSC. MPMC would add unnecessary complexity.
- **Capacity:** 8192 events (configurable at compile time)

### D3: Atomic note tracking vs hash map
- **Chosen:** Bitmask (2 × uint64 per channel)
- **Rationale:** O(1) note on/off, O(1) panic clear, no allocation. 128 notes × 16 channels = 2,048 bits.
- **Tradeoff:** No per-note velocity tracking for note-stealing. Acceptable for Phase 1.

## Open Recommendations

1. Add `mach_absolute_time()` backend for real device timing (deferred to Gate 2 with CoreAudio hook)
2. Add memory barrier stress tests for the lock-free queue (deferred to Gate 8 stability hardening)
3. Validate scheduler dispatch timing inside actual CoreAudio render callback (Gate 2)

## Gate 1 Exit Criteria

| Criterion | Status |
|-----------|--------|
| Build + tests pass | ✅ 46/46 tests pass |
| Scheduler simulation works | ✅ Non-blocking, tick-accurate |
| Timing benchmark generated | ✅ P99 = 0.12 µs |
| Panic test passes | ✅ 12/12 panic tests pass |
| Architecture review completed | ✅ This document |
| No vendor-specific runtime contamination | ✅ Confirmed |
| No silent MegaVoice→GM mapping | ✅ Confirmed |
