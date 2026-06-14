# Musical Review — GATE 9 (Multi-Playback Validation & Musical Feel)

> **Date:** 2026-06-14
> **Branch:** `gate-9-multi-playback-validation` (includes Gate 8 merge)
> **Style tested:** Gate 2 Demo Style (Rock) — 4 sections, 120 BPM + empty-section style
> **Playback methods:**
>   - Offline virtual-clock validation harness (automated, CI)
>   - CoreMIDI external output path (`korg-playback` CLI) — manual, requires hardware
> **Status:** CONDITIONAL PASS — engine-verifiable items PASS; Groove & Articulation final confirmation pending external Korg/Yamaha.

---

## How to read this scorecard

Each category is scored 0–5. Gate 9 acceptance: **TOTAL ≥ 15 and no category < 2.**
Two columns:
- **Engine-verified** — proven by the automated harness (deterministic, repeatable on CI).
- **Korg-manual** — requires ears on a real arranger; run `build/korg-playback` on a Mac with the device connected (see §6). *Sound fidelity is explicitly NOT evaluated in Gate 9.*

---

## 1. Fill Transition Feel — Engine-verified

| Transition | Result | Evidence |
|-----------|--------|----------|
| Intro → Main | ✅ PASS | switch on bar boundary (`test_playback_validation` §3) |
| Main → Fill | ✅ PASS | state sequence assertion |
| Fill → Main (auto-return) | ✅ PASS | fill played for declared `section.bars` then returned, **0 stuck notes** (§5) |
| Main → Ending | ✅ PASS | state sequence assertion |
| Mid-section (mid-note) switch | ✅ PASS | flush-on-switch → **0 stuck notes** on rapid switching |

**Score: 4/5** — all transition *mechanics* verified and balanced; the subjective "feel" of fills still benefits from ears (Korg).

## 2. Chord Correctness — Engine-verified (+ G8)

| Aspect | Result | Evidence |
|--------|--------|----------|
| Root transposition | ✅ PASS | `transposeNote()` maps to chord root |
| Chord-tone (root/fifth) voicing | ✅ PASS (G8) | Major → root+4/+7, Minor → root+3/+7 |
| Chord change without stuck notes | ✅ PASS | `setChord()` flushes notes voiced under previous chord |
| Voicing collision handling | ✅ PASS | dedupe NoteOn/NoteOff against active set (two source voices → one MIDI note no longer leaks) |
| NTR/NTT semantic tables | ⚠️ PARTIAL | NTR/NTT carried in `ArticulationMetadata` (G8) but full NTT table application is simplified |

**Score: 3/5** — chord-aware transposition correct & balanced; full NTT tables remain future work (Gate 10).

## 3. Groove Preservation

| Element | Engine-verified | Korg-manual |
|--------|----------------|-------------|
| Tempo tick↔time accuracy | ✅ PASS — 24000 samples == 480 ticks exactly, bar math exact | — |
| Dispatch timing / latency | ✅ PASS — schedule→dispatch p50 ≈ 200 µs, **p99 ≈ 385 µs**, no event loss | confirm no audible lag on device |
| Event ordering | ✅ PASS — FIFO SPSC, balanced across reconnect | — |
| Swing / shuffle / push-pull | ❌ not implemented | feel pending |

**Score: 3/5 (engine), feel PENDING Korg** — timing is provably accurate; micro-groove/swing is not modelled yet.

## 4. Articulation Fidelity

| Type | Engine-verified | Korg-manual |
|------|----------------|-------------|
| NTR/NTT metadata plumbed | ✅ mapped (G8) | — |
| Keyswitch / MegaVoice render | ❌ not rendered | required on device |
| Velocity map | ⚠️ partial (CASM) | required on device |

**Score: 2/5, PENDING Korg** — metadata present; articulation rendering requires the external instrument.

## 5. Arranger Behavior — Engine-verified

| Behavior | Result | Evidence |
|----------|--------|----------|
| Section switch on bar boundary | ✅ PASS | distinct increasing bars, offset < 64 ticks |
| intro→main→fill→main→ending order | ✅ PASS | state-sequence equality assertion |
| Fill length == `section.bars` | ✅ PASS | declared length honoured |
| Every track dispatches ≥1 event | ✅ PASS | bass(ch1)/chord(ch2)/drums(ch9) all covered |
| Polyphony | ✅ PASS | overlapping chord notes observed |
| Panic / all-notes-off | ✅ PASS | `flushActiveNotes` + CC120/123; **0 stuck** |
| Full cycle + stop balance | ✅ PASS | NoteOn count == NoteOff count, **0 stuck, 0 orphan** |
| Empty / silent sections | ✅ PASS | no events, no crash |
| Hotplug disconnect/reconnect | ✅ PASS | every event delivered in order, 0 stuck |

**Score: 5/5.**

## 6. Overall Verdict

| Category | Score | Basis |
|----------|-------|-------|
| Fill transition feel | 4/5 | engine-verified |
| Chord correctness | 3/5 | engine-verified + G8 |
| Groove preservation | 3/5 | timing engine-verified; swing pending |
| Articulation fidelity | 2/5 | metadata only; render pending Korg |
| Arranger behavior | 5/5 | engine-verified |
| **TOTAL** | **17 / 25** | ≥ 15 ✓, no category < 2 ✓ |

**VERDICT: CONDITIONAL PASS (17/25).**
All engine-verifiable musical-correctness properties pass deterministically. The two
P1 items that genuinely need a human ear on hardware — **Groove preservation (swing/feel)**
and **Articulation fidelity (keyswitch/MegaVoice rendering)** — are scored on their
engine-side readiness and flagged PENDING the manual Korg pass. **Sound fidelity was not
evaluated, per scope.**

### Manual Korg pass — checklist (run `build/korg-playback`)
1. Connect Korg PA / Yamaha Genos via USB/MIDI; launch `build/korg-playback` (or pass a `.uasf`).
2. Press `[` / `]` to select the device, `space` to start.
3. Cycle `1`–`6` sections, `f` fill, `e` ending; listen for: groove preserved, fills land on the bar, no hung notes.
4. Press `c` to cycle chords; confirm re-voicing is clean (no stuck notes — engine flushes on chord change).
5. Press `p` (panic) mid-phrase; confirm instant silence.
6. Note articulation behaviour (keyswitch/MegaVoice) — expected gaps documented above.
7. On exit the CLI prints final NoteBalance (`stuck=0` expected) and schedule→dispatch latency.

> Fill in observed results here after the hardware session:
>
> | Item | Observed | OK? |
> |------|----------|-----|
> | Groove preserved across sections | | |
> | Fills land on bar boundary (audible) | | |
> | No hung notes through full cycle | | |
> | Chord re-voicing clean | | |
> | Panic = instant silence | | |
> | Articulation (keyswitch/MegaVoice) | | |
