# Codex Review: GATE 1 — Realtime Timing Spike

**Branch:** gate-1-realtime-timing-spike
**SHA:** d6e9e8a
**Review Date:** 2026-06-13
**Reviewer:** Codex

---

## 1. Architecture Compliance

| Rule | Status | Evidence |
|------|--------|----------|
| No malloc in realtime path | ✅ PASS | LockFreeQueue uses std::array (stack); no heap in clock/scheduler/panic |
| No mutex in realtime path | ✅ PASS | All sync via std::atomic (lock-free SPSC, atomic position/state) |
| No file IO in realtime callback | ✅ PASS | IO only in benchmark_report (non-realtime) |
| No ObjC/Swift/ARC in realtime path | ✅ PASS | Pure C++20, no Foundation references |
| Runtime knows only UASF types | ✅ PASS | Only uasf::MidiEvent referenced |
| No Yamaha/Korg logic | ✅ PASS | Zero vendor references |
| No silent MegaVoice→GM mapping | ✅ PASS | MegaVoice deferred; articulation types exist |

## 2. Code Quality

| File | Lines | Assessment |
|------|-------|------------|
| `src/engine/realtime/clock.h/.cpp` | 150 | ✅ Clean atomic state machine |
| `src/engine/midi/scheduler.h/.cpp` | 105 | ✅ Lock-free SPSC, std::function callback OK for non-realtime init |
| `src/engine/midi/panic.h/.cpp` | 120 | ✅ Note tracking via atomic bitmask (O(1)), no dynamic allocation |
| `src/engine/realtime/timing_benchmark.h/.cpp` | 161 | ✅ Clear percentile calculation |
| `src/engine/uasf/types.h` | 120 | ✅ Clean UASF type definitions, ADR-013 articulation metadata |

### Findings

1. **std::function in scheduler callback** — std::function may allocate in some implementations. Recommendation: replace with raw function pointer for Gate 2 realtime path.

2. **PanicHandler::noteOff CAS loop** — Compare-exchange loop is correct but could spin under contention. Acceptable for Phase 1; consider ticket lock for Gate 8 hardening.

## 3. Test Coverage

| Suite | Tests | Coverage |
|-------|-------|----------|
| test_clock | 9/9 ✅ | Default state, advance, position, bar/beat |
| test_scheduler | 14/14 ✅ | Push/pop, callback, clear, overflow |
| test_panic | 12/12 ✅ | On/off tracking, panic dispatch, edge cases |
| test_timing_benchmark | 11/11 ✅ | Metrics validity, percentile ordering, buffer sizes |

## 4. Timing Metrics

**P99 jitter: 0.12 us (target < 1000 us) ✅ PASS**

**CAUTION:** Simulation-only benchmark (std::chrono::high_resolution_clock, no real CoreAudio callback). Real device timing may differ.

## 5. Verdict

**VERDICT: PASS** ✅

**P1 Blockers:** None
**P2 Risks:** std::function allocation in scheduler callback (low)
**MERGE_ALLOWED:** YES
**Required before Gate 2:** std::function to raw pointer callback
