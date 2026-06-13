# GATE 7 — CASM Semantic Analysis & Musical Playback Validation

> **Status:** IN PROGRESS  
> **Branch:** gate-7-casm-semantic-analysis  
> **Prerequisite:** Gate 6 ✅ (corpus validation, SFF2/SMF support)

---

## Objective

Analyze CASM semantics, map chord behavior (NTR/NTT), play back converted UASF, and create first real Musical Review.

## Critical Rule

A style that parses perfectly but behaves musically wrong is NOT compatible.
Do NOT claim "Yamaha support" until semantic fidelity is validated.

## Scope

- CASM data structure analysis (NTR, NTT, key ranges, MegaVoice)
- Chord behavior mapping from CASM → UASF articulation
- Real playback test of converted CLASSIC_6_8.uasf via external MIDI
- Musical Review (first real one, per MUSICAL_REVIEW_TEMPLATE.md)
- Semantic compatibility metrics (harmonic, groove, arranger behavior, usability)

## Non-Goals

- Full NTT table implementation (Gate 8+)
- MegaVoice rendering
- SFF2
- UI

## Tasks

| Task | Output |
|------|--------|
| G7-T01 | CASM binary structure notes (`docs/importers/casm-structure-notes.md`) |
| G7-T02 | CASM parser: extract NTR, NTT, key ranges, MegaVoice flags |
| G7-T03 | Chord behavior mapping report (NTR = Root, Chord, Melodic, etc.) |
| G7-T04 | Real playback test: UASF → CoreAudio → external MIDI (or AUv3) |
| G7-T05 | Musical Review (per template: fill, chord, groove, articulation, arranger) |
| G7-T06 | Semantic compatibility scoring (harmonic, groove, arranger, usability) |
| G7-T07 | Unsupported arranger behaviors taxonomy |

## Exit Criteria

- [ ] CASM structure documented
- [ ] CASM data extracted from real .sty files
- [ ] NTR/NTT behavior at least documented (even if not implemented)
- [ ] Playback test attempted (even if degraded)
- [ ] Musical Review scorecard filled
- [ ] Semantic compatibility score computed
- [ ] Unsupported behaviors classified
- [ ] No unsupported compatibility claim
- [ ] Build + tests pass
- [ ] Codex review pass
