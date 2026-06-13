# GATE 9 — Multi-Playback Validation & Musical Feel Review

> **Objective:** Validate UASF playback correctness through available hardware (Korg, not Yamaha).
> **Branch:** gate-9-multi-playback-validation
> **Hardware available:** Korg keyboard (not Yamaha)
> **Prerequisite:** Gate 8 ✅ (NTR/NTT semantics, chord-aware transpose)

---

## Critical Rule

For Yamaha-imported styles played through Korg:
- Judge ONLY arrangement behavior: timing, section transitions, chord correctness, groove, track routing
- Do NOT judge Yamaha sound fidelity (different synth engine)
- This is a **behavioral compatibility test**, not a sound fidelity test

Yamaha hardware validation is deferred to Gate 9B / Gate 10 pre-beta.

## Available Test Targets

| Target | Priority | Notes |
|--------|----------|-------|
| Internal/simulated playback | P0 | Always available, timing verified |
| MIDI out to Korg hardware | P1 | Primary musical evaluation path |
| AUv3/SF2 if available | P2 | Optional, improves sound quality |

## Scope

1. **UASF playback correctness**
   - Load converted .uasf styles from corpus
   - Verify all tracks play
   - Verify tempo accuracy
   - Verify section switching

2. **Section switching feel**
   - Intro → Main → Fill → Ending transitions
   - Bar-boundary accuracy
   - Fill duration correctness
   - Ending decay/cut behavior

3. **Chord transformation**
   - Root shift on chord change
   - Fifth tracking
   - Bass movement
   - Inversion handling (if NTT supports it)

4. **MIDI out stability**
   - No stuck notes
   - No orphan NoteOn
   - Hotplug/reconnect handling
   - Latency measurement

5. **Panic/stuck note recovery**
   - Panic clears all sound
   - Reset after panic resumes correctly

6. **Korg external playback validation**
   - Connect Korg keyboard via USB or MIDI
   - Play converted .uasf styles
   - Evaluate timing, transitions, chords

7. **Musical Review (REQUIRED)**
   - Per MUSICAL_REVIEW_TEMPLATE.md
   - Fill transition feel
   - Chord correctness
   - Groove preservation
   - Articulation fidelity
   - Arranger behavior

## Non-Goals

- Yamaha sound fidelity evaluation (deferred to Gate 10)
- Internal sound engine
- MegaVoice rendering
- Full SFF2 support claim
- Marketplace/cloud/AI

## Success Criteria

- [ ] Converted .uasf styles play through Korg hardware
- [ ] Section switching on correct bar boundaries
- [ ] Chord changes affect playback deterministically
- [ ] No stuck notes during 5-minute session
- [ ] Panic recovers cleanly
- [ ] Musical Review scorecard completed
- [ ] Timing verified on real MIDI path
- [ ] Known behavior mismatches documented

## Musical Review Evaluation (for Yamaha→Korg)

| Category | Expectation | Notes |
|----------|-------------|-------|
| Timing | ✅ Must be correct (engine verified) |
| Section transitions | ✅ Must be bar-accurate |
| Chord correctness | ✅ Root/fifth shifts verified |
| Groove preservation | ⚠️ Korg GM sound may differ from Yamaha |
| Track routing | ✅ Drums→Ch9, Bass→Ch1 verified |
| Arranger behavior | ✅ Intro/Main/Fill/Ending verified |
| Sound fidelity | ❌ NOT evaluated (different hardware) |
