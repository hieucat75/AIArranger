# Gate 9 — P1 Open Items

> Tracked after Gate 9 close-out (merge `edb6a20`, tag `v0.9.0-gate9`).
> These do **not** block the Gate 9 merge (engine-verifiable validation passed),
> but gate the move from **CONDITIONAL PASS → FULL PASS** and feed Gate 10.

| # | Item | Type | Owner | Resolution path |
|---|------|------|-------|-----------------|
| ☐ 1 | **Groove preservation on real Korg hardware** | Manual review | PTH | Run `build/korg-playback` on the Korg; score 0–5 (see manual checklist) |
| ☐ 2 | **Articulation fidelity / keyswitch / MegaVoice behavior** | Manual review | PTH | Listen on Korg; score 0–5; note rendering gaps |
| ☑ 3 | **Swing / shuffle not modeled** | Engine deferral | ✅ Gate 10B (Task B) | Added `src/engine/music/groove::applySwing` (off-beat 8th delay, 50–75%); StylePlayer `setSwing()`. Per-style feel to be tuned on hardware. See `GATE_10B_HANDOFF.md`. |
| ☑ 4 | **NTT tables simplified (root/fifth only)** | Engine deferral | ✅ Gate 10 (Task B) | Implemented full table-driven NTR/NTT in `src/engine/music/ntt` (Bass/Chord/Melody + Melodic/Harmonic/Natural minor + Dorian). Guitar/SFF2 fall back to RootTransposition with a log. See `GATE_10_HANDOFF.md`. |
| ☑ 5 | **Intro one-bar delay semantics** | Document / revisit | ✅ Gate 10B (Task C) | Was a sequencer bug, not spec: queued intro waited for a bar boundary so it began a bar late. Fixed with an immediate first-section start path (later changes still quantise to the bar). Asserted by `test_section_sequencer`. See `GATE_10B_HANDOFF.md`. |

## Notes

- Items **1–2** are the only blockers for FULL PASS; they require a human ear on hardware.
  After the session, record the scores into `.openclaw/review/GATE_9_MUSICAL_REVIEW.md`
  (§6 table) and flip the Gate status in `HANDOFF_MASTER.md`.
- Items **3–5** are engineering follow-ups carried into Gate 10 scope — not required for
  FULL PASS of Gate 9, listed here so they are not lost.
- **Sound fidelity is explicitly out of scope** for Gate 9 (and is not an open item here).
