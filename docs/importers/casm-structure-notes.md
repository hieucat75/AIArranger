# CASM Structure & Semantic Analysis

> **Status:** VALIDATED (Gate 7)  
> **Branch:** gate-7-casm-semantic-analysis  
> **Source:** Byte-level analysis of 4 Yamaha Genos .S718 style files
> (CLASSIC_6_8, C_WHISPER, BALLAD_FOLK, POP_ACOUSTIC_2)

---

## CASM Binary Structure (SFF2/SMF embedded)

```
CASM chunk (variable size, ~3-4 KB in Genos files)
└── CSEG (outer)  "CSEG" + 4-byte big-endian length
    ├── Sdec  section name ("Main A")           ┐ section 1 lives directly
    ├── Ctb2  track config (Rhythm2)             │ in the outer CSEG
    ├── Ctb2  track config (Bass) ...            ┘
    ├── CSEG (inner)  ── section 2 ──
    │   ├── Sdec  "Main B"
    │   └── Ctb2 × N
    ├── CSEG (inner)  ── section 3 ── ...
    └── ...
```

- Each **Sdec** sub-chunk names one arranger section. Observed names:
  `Main A..D`, `Fill In AA..DD`, `Fill In BA`, `Intro A/B`, `Ending A`.
- The **Ctb2** sub-chunks that follow an Sdec (within the same CSEG)
  are that section's source-channel track configs. Section 1 is not
  wrapped in its own inner CSEG; sections 2+ are.
- Sub-chunk framing is `id[4] + size[4 big-endian] + data[size]`.

## Ctb2 Track Configuration (47 bytes per block — VALIDATED)

```
Offset  Field
[0]     Source Channel        (09 = ch9 / GM drums, 0a, 0b, ...)
[1..8]  Name (8 bytes, space-padded)   "Rhythm2 ", "Bass    "
[9]     Destination Channel
[10]    Editable flag
[11..17] Note / chord mute bitfields
[18..19] Source chord (root=00 C, type=02)
[20]    Note Limit Low
[21]    Note Limit High
[22]    NTR  (Note Transposition Rule)
[23]    NTT  (bit 7 = bass-note flag; bits 0-6 = table type)
[24..]  Repeats for additional section variants (ignored — Gate 8+)
```

> The earlier draft put the name at bytes 0-19 and NTT at byte 5; both
> were wrong. The leading byte is the source channel (the `0x09` that
> looked like a tab on drum tracks), and NTT lives at byte 23. Reading
> byte 25 produced garbage (e.g. 0x1c = 28 on Bass).

## NTR Values Observed

| Value | Meaning | Tracks |
|-------|---------|--------|
| 0 | Root (root-relative) | Bass, Chord1, Pad, Phrase1 |
| 1 | Fifth / root+fifth    | Rhythm1, Rhythm2, Chord2, Phrase2 |

## NTT Values Observed (bits 0-6, with bit 7 = bass flag)

| Type | Meaning | Tracks |
|------|---------|--------|
| 0 | Bypass | Rhythm1/Rhythm2 (drums), Strings (some) |
| 1 | Melody / default | Phrase1, Phrase2; Bass (with bass flag → 0x81) |
| 2 | Chord | Chord1, Chord2, Pad, E.Piano |

> NTT type indices here are the **raw on-disk values**. They do not
> match the placeholder `NttType` enum in sff1_types.h (which predates
> this analysis). Full NTT transposition-table semantics are Gate 8+.

## Track Role Mapping (Genos SFF2)

| CASM Track | MIDI Channel | Role |
|-----------|-------------|------|
| Rhythm2 | 9 (10) | Drums |
| Bass | varies | Bass |
| Chord1 | varies | Chord |
| Chord2 | varies | Chord (alternative voicing) |
| Pad | varies | Pad/background |
| Phrase1 | varies | Melodic phrase |
| Phrase2 | varies | Melodic phrase (alt) |

## MegaVoice Detection

MegaVoice patterns are indicated by:
- Specific velocity ranges in MIDI data (>60 range)
- NTT value specific to articulation switching
- CASM velocity tables (if present)

## Semantic Mapping to UASF (Gate 7 implementation)

| CASM Concept | UASF Field | Status |
|-------------|-----------|--------|
| Section (Sdec) | SectionDefinition.type/name | ✅ `mapCasmSectionType` |
| Track role | TrackRole (name-primary + bass flag) | ✅ `mapCasmTrackRole` |
| Source channel | TrackDefinition.midi_channel | ✅ |
| NTR | — | ❌ No UASF v1 field — logged unmapped |
| NTT | — | ❌ No UASF v1 field — logged unmapped |
| Note range | track.high_key/low_key | ❌ Not in UASF v1 |
| MegaVoice | articulation.profile | ⚠️ velocity-derived (report), ADR-013 |

Role inference is **name-primary**: the Ctb2 track name is the most
reliable role signal in real files. NTR/NTT refine it only via the NTT
bass-note flag (→ Bass). NTR/NTT themselves are not representable in
UASF v1 and are recorded in `SffToUasfResult::unmapped_features` rather
than silently dropped (architecture rule #5). MIDI events stay in the
single SMF MTrk — splitting them across CASM sections is Gate 8+.

## Fidelity Impact

| Missing Feature | Impact | Priority |
|----------------|--------|----------|
| NTR/NTT mapping | Chord behavior wrong on non-root chords | HIGH |
| Note range filtering | Notes played outside instrument range | MEDIUM |
| Velocity mapping | Articulation not triggered | MEDIUM |
| Section detection | No Intro/Fill/Ending detection | HIGH |
