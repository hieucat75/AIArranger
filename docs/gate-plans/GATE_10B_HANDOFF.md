# Gate 10B — Handoff (Hardware + Swing + NTT validation)

> Branch `gate-10b-hardware-swing-ntt-validation` off `main` (`6ab8668`),
> merged via PR #11 (`26d8e03`), tag `v0.10.1-gate10b-engine`.
> **Status: ✅ Gate 10B Engine PASS / 🅓 Hardware DEFERRED_BY_PTH.**
> 22 test binaries, 345 assertions, 0 failures. CI (GitHub Actions) green.
> This closes the **engine-side** of Gate 10B. The **hardware-side** (manual
> validation + NTT A/B calibration on a real Yamaha/Korg) was **explicitly
> deferred by PTH on 2026-06-14** — docs are preserved, not in progress. **Not**
> "Gate 10B FULL PASS"; hardware parity is **not claimed**
> (see `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`).
>
> **Reactivation:** when PTH or a future maintainer signals readiness, re-open
> `GATE_10B_HARDWARE_CHECKLIST.md` + `GATE_10B_NTT_CALIBRATION.md` and run them on
> a real device — **no code changes required** to resume.

---

## 1. Split: engine-side (done here) vs hardware-side (pending PTH)

| # | Item | Side | Status |
|---|------|------|--------|
| 2 | Swing / shuffle timing | engine | ✅ Task B |
| 3 | Intro one-bar delay fix | engine | ✅ Task C |
| 5 | Guitar NTT + MegaVoice fallback | engine | ✅ Task E |
| 6 | Per-section split validation | engine | ✅ Task F |
| 1 | Yamaha/Korg hardware validation | hardware | ⏳ checklist ready (Task A doc) |
| 4 | NTT A/B calibration | hardware | ⏳ procedure ready (Task D doc) |

---

## 2. Commits

| Hash | Title |
|---|---|
| `f7f891e` | G10B-TASK-B: swing/shuffle timing model |
| `681c834` | G10B-TASK-C: fix intro one-bar delay (immediate first-section start) |
| `b6885ee` | G10B-TASK-E: guitar NTT + MegaVoice fallback |
| `0cc87ef` | G10B-TASK-F: per-section split validation across full corpus |
| `9199e5a` | G10B-TASK-A: hardware checklist + NTT calibration docs |

(+ this handoff commit.)

---

## 3. What changed (engine)

### Task B — Swing / shuffle (`src/engine/music/groove.{h,cpp}`)
- `applySwing(tick, ticksPerBeat, swingPercent)` — off-beat 8ths slide later by
  `(swing% − 50%) × ticksPerBeat`; on-beat positions hold; swing clamped to
  [50, 75]. 50 % = straight (default). Pure, RT-safe (integer math).
- `StylePlayer` applies swing to each dispatched event's absolute tick;
  NoteOn/NoteOff in the same off-beat half shift equally (pairs stay balanced).
  Live-settable via `setSwing()` (atomic). Default 50 → playback unchanged.

### Task C — Intro one-bar delay fix (`section_sequencer`)
- **Root cause (G9 P1 #5):** `advance()` only activated a queued section on a bar
  boundary (`currentBar > lastBar`). At START the intro is queued at tick 0 where
  `0 > 0` is false, so it didn't begin until bar 2 — a bar of silence.
- **Fix:** when nothing is playing yet (`current_section_ < 0`), activate the
  queued section immediately; later section changes still quantise to the next
  bar boundary. Shared `activateSection()` helper. **Verdict: sequencer bug, not
  Yamaha spec — corrected.**

### Task E — Guitar NTT + MegaVoice fallback
- `NttMode::Guitar` added: voices recorded notes onto target chord tones (guitar
  comping); `NtrRule::Guitar` handles the root (v1 = root transposition). No
  longer a silent fall-through to Melody.
- `KeyswitchRenderer(bool keyswitchAvailable)`: when the target can't honour
  keyswitch, gracefully degrade to pass-through (note still sounds). Per the
  no-silent-fallback rule the **caller logs** the degradation at construction
  (non-RT); `render()` stays RT-safe. `name()` reports `keyswitch-fallback`.

### Task F — Corpus-wide split validation
- `test_corpus_split_validation` iterates every `.sty` under `corpus/yamaha/sff1`
  and asserts the split invariants (≥4 per-role tracks, bass + chord + a rhythm
  track, each track single-channel). Validated 4 Genos styles (16/9/9/8 tracks).

---

## 4. Hardware-side docs (Task A, for PTH)

- `GATE_10B_HARDWARE_CHECKLIST.md` — step-by-step `korg-playback` validation on a
  real device (split audibility, intro start, chord/NTT, swing feel,
  articulation, panic). Scorecard feeds `GATE_9_MUSICAL_REVIEW.md` §6.
- `GATE_10B_NTT_CALIBRATION.md` — A/B procedure with the engine's **reference NTT
  table** (bass tracks chord root; chord voicings are exact chord tones across
  C/F/G/Am/Dm/Em/Bdim), to compare note-by-note against a real Yamaha and log
  discrepancies for table tuning.

---

## 5. Test evidence

| Suite (new/changed in Gate 10B) | Assertions |
|---|---|
| `test_groove` (new) | 12 |
| `test_guitar_megavoice` (new) | 11 |
| `test_corpus_split_validation` (new) | 25 |
| `test_section_sequencer` (+5 immediate-start) | 17 (was 12) |
| **Gate 10B new total** | **+53** |

- Full suite: **22 binaries, 345 assertions, 0 failures** (was 19 / 292).
- Corpus dispatch unchanged at 3876 (swing default 50, intro fix shifts timing
  but not totals).
- CI (GitHub Actions, macos-latest) green on the branch.

---

## 6. Decisions worth noting

1. **Swing model = fixed off-beat-half delay**, not per-note micro-timing. The
   whole second half of each beat slides by one delay, preserving internal
   spacing — simple, deterministic, testable, musically reasonable. Calibrate
   the exact feel on hardware (checklist §5).
2. **Intro delay was a bug, not spec.** Fixed with an immediate-start path for
   the first section only; subsequent changes keep bar-boundary quantise.
3. **Guitar NTT v1 = chord-tone voicing.** Treated like the chord table on its
   own track; faithful guitar register/voicing is future work.
4. **MegaVoice degrade is explicit + logged at config**, never silent (rule 5/7).
   The RT renderer just reads an immutable flag.
5. **NTT engine confirmed musically correct** during calibration-doc prep (a
   transient "collapsed" probe output was a static-buffer bug in a throwaway
   generator, not the engine — `test_ntt` pins the real values).

---

## 7. Residual risks / still pending

- **Hardware validation (#1) + NTT calibration (#4)** — **DEFERRED_BY_PTH
  (2026-06-14)**. Docs preserved; results not captured. Hardware parity is NOT
  claimed anywhere (see `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`).
- **Swing feel** is theory-tuned; the right per-style percentage is a hardware
  call.
- **Guitar/MegaVoice** are v1 approximations (chord voicing / keyswitch-or-
  bypass), not full Yamaha MegaVoice rendering.
- **Per-section (marker-based) event splitting** still not implemented — split is
  per-channel; CASM section structure remains separate from the SMF MTrk.
- **Swing has no UI/serialised field yet** — runtime `setSwing()` only; not
  persisted in UASF.

---

## 8. Merge status

**MERGED** — PR #11 approved by PTH and merged to `main` (merge SHA `26d8e03`),
tag `v0.10.1-gate10b-engine`. Post-merge suite on `main`: 22 binaries / 345
assertions / 0 failures; CI green.

> Gate 10B **engine** slice is complete, merged, and green. Gate 10B **hardware**
> slice (checklist + calibration) is **DEFERRED_BY_PTH** — preserved as a future
> milestone, reactivate on a physical device with no code changes.
