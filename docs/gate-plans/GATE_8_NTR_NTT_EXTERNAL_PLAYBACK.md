# GATE 8 — NTR/NTT Semantic Implementation & External MIDI Playback

> **Objective:** Musical semantics, not parsing. Make the arranger "behave like a real arranger."
> **Branch:** gate-8-ntr-ntt-and-external-playback
> **Prerequisite:** Gate 7 ✅ (CASM analysis, 83.6% harmonic fidelity)

---

## Priority Order

1. NTR/NTT research + implementation
2. External MIDI playback with real device
3. Harmonic fidelity metrics expansion
4. Musical Review with evidence

## A. NTR/NTT Implementation

NTR (Note Transposition Rule) determines how a style note moves when chord changes:
- Root=0: Note maps to chord root (fixed)
- Fifth=1: Note maps to chord root + 5th
- Chord=2: Note maps to nearest chord tone
- Bass=3: Scale-based bass movement
- Fixed=6: No transposition (drums/percussion)

NTT (Note Transposition Table) determines the voicing algorithm:
- Default=1: Standard melodic
- Guitar=2: Guitar voicing
- Bass=4: Bass voicing
- Perc=6: Percussion (no transposition)
- SFF2=7: Yamaha SFF2 extension

Implementation must:
- Document assumptions explicitly
- Support only verified behaviors first
- Reject unknown semantics loudly
- NOT fake unsupported behavior

## B. External MIDI Playback

Connect real Yamaha device OR AUv3 instrument to Mac.
Play converted UASF styles through external device.
Compare: chord behavior, fill behavior, bass movement, tempo fidelity.

## C. Harmonic Fidelity Metrics

Expand from current 83.6%:
- Chord correctness
- Bass correctness  
- Inversion stability
- Cadence behavior
- Transition realism

## D. Musical Review

REQUIRED, not optional.
Evidence needed: recordings, side-by-side comparison, reviewer notes.

## Non-Goals
- Full SFF2 support claim
- MegaVoice rendering
- Internal sound engine
- Marketplace/cloud/AI
