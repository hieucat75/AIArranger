# Gate 10B — NTT A/B Calibration (for PTH)

> **Manual gate — requires a real Yamaha arranger.** Goal: compare the engine's
> NTT output against an authentic Yamaha playing the **same style + same chord
> progression**, note-by-note, and log discrepancies so the engine tables can be
> tuned by ear/data. The NTT engine is *implemented and unit-tested*; it is NOT
> yet validated against real Yamaha voicing — that is what this procedure does.

## Why
`src/engine/music/ntt` builds its transposition from music theory (see
`arranger-engine-spec-v0.9.md §6.3`). Yamaha's exact tables are proprietary and
musically nuanced. The plan (spec §6.3): theory baseline → A/B vs reference →
adjust. This doc is the A/B step.

---

## A. Engine reference output (current tables)

Source reference chord = C (root MIDI 60). Recorded probe notes:
bass **root C2 = 36**, bass **fifth G2 = 43**; chord triad **C/E/G = 60/64/67**.
NTR = RootTransposition. The engine currently produces:

| Chord | Bass root (rec C2=36) | Bass fifth (rec G2=43) | Chord triad (rec C/E/G=60/64/67) |
|---|---|---|---|
| C    | C2 (36) | G2 (43) | C4 / E4 / G4 |
| F    | F2 (41) | C3 (48) | F4 / A4 / C5 |
| G    | G2 (43) | D3 (50) | G4 / B4 / D5 |
| Am   | A2 (45) | E3 (52) | A4 / C5 / E5 |
| Dm   | D2 (38) | A2 (45) | D4 / F4 / A4 |
| Em   | E2 (40) | B2 (47) | E4 / G4 / B4 |
| Bdim | B2 (47) | F3 (53) | B4 / D5 / F5 |

> Regenerate this table any time the tables change — the values come straight
> from `music::transpose(...)` (the same function the runtime uses). The
> `test_ntt` suite locks the C/F/G/Am/Dm bass column.

---

## B. Capture the Yamaha reference

1. On the Yamaha, load the **same style** (the original `.S718`/`.sty` the corpus
   came from) and set it to **Main A**.
2. Route the Yamaha **MIDI OUT** → Mac (USB-MIDI). Open a MIDI monitor (e.g. CoreMIDI
   monitor, or `korg-playback`'s input log if available).
3. For each chord in the progression **C → F → G → Am → Dm → Em → Bdim**:
   - Play that chord in the Yamaha's chord-detect zone (left hand / ACMP on).
   - Record the actual MIDI notes the Yamaha emits on the **bass** part and the
     **chord** part for one bar.
   - Note the lowest bass note and the chord voicing (pitch classes).

## C. Compare + log discrepancies

For each chord, fill the observed Yamaha notes next to the engine reference:

| Chord | Part | Engine | Yamaha (observed) | Δ (semitones) | Acceptable? |
|---|---|---|---|---|---|
| C  | bass root | C2 | | | |
| C  | chord     | C/E/G | | | |
| F  | bass root | F2 | | | |
| F  | chord     | F/A/C | | | |
| G  | bass root | G2 | | | |
| G  | chord     | G/B/D | | | |
| Am | bass root | A2 | | | |
| Am | chord     | A/C/E | | | |
| Dm | bass root | D2 | | | |
| Dm | chord     | D/F/A | | | |
| Em | bass root | E2 | | | |
| Em | chord     | E/G/B | | | |
| Bdim | bass root | B2 | | | |
| Bdim | chord   | B/D/F | | | |

**Interpretation**
- Δ = 0 on bass root and chord tones → engine matches Yamaha for that chord.
- Octave-only differences (Δ = ±12) → acceptable (register choice), note it.
- Wrong pitch class (Δ ∉ {0, ±12}) → a real NTT discrepancy → log below.

## D. Discrepancy → engine adjustment

For each non-octave discrepancy, record:
```
chord=____ part=____ engine=____ yamaha=____ note=____
proposed fix: (e.g. "minor chords should keep recorded 9th as tension, not snap")
```
Hand these back to engineering. Adjustments land in `music::transpose` /
`chordTones` / `scaleFor`, with a new `test_ntt` case pinning the corrected value.

---

## Verdict

- **NTT calibration result:** MATCH / MINOR-DRIFT / NEEDS-TUNING — ____
- **Device + style used:** ____   **Date:** ____
- Discrepancies logged: ____ (attach list)
