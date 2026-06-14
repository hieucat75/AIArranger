# Gate 9 — P1 Open Items

> Tracked after Gate 9 close-out (merge `edb6a20`, tag `v0.9.0-gate9`).
> These do **not** block the Gate 9 merge (engine-verifiable validation passed),
> but gate the move from **CONDITIONAL PASS → FULL PASS** and feed Gate 10.

| # | Item | Type | Owner | Resolution path |
|---|------|------|-------|-----------------|
| ☐ 1 | **Groove preservation on real Korg hardware** | Manual review | PTH | Run `build/korg-playback` on the Korg; score 0–5 (see manual checklist) |
| ☐ 2 | **Articulation fidelity / keyswitch / MegaVoice behavior** | Manual review | PTH | Listen on Korg; score 0–5; note rendering gaps |
| ☐ 3 | **Swing / shuffle not modeled** | Engine deferral | Gate 10 | Add groove/swing model to timing path |
| ☐ 4 | **NTT tables simplified (root/fifth only)** | Engine deferral | Gate 10 | Implement full NTT tables (Guitar/Bass/Scale/Chord/Percussion) |
| ☐ 5 | **Intro one-bar delay semantics** | Document / revisit | Gate 10 | Sequencer activates a queued section on the *first* bar boundary (asserted by `test_section_sequencer`). Decide: keep & document, or add an immediate-start path |

## Notes

- Items **1–2** are the only blockers for FULL PASS; they require a human ear on hardware.
  After the session, record the scores into `.openclaw/review/GATE_9_MUSICAL_REVIEW.md`
  (§6 table) and flip the Gate status in `HANDOFF_MASTER.md`.
- Items **3–5** are engineering follow-ups carried into Gate 10 scope — not required for
  FULL PASS of Gate 9, listed here so they are not lost.
- **Sound fidelity is explicitly out of scope** for Gate 9 (and is not an open item here).
