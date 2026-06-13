# CASM Structure & Semantic Analysis

> **Status:** DRAFT  
> **Branch:** gate-7-casm-semantic-analysis  
> **Source:** Analysis of 3 Yamaha Genos .S718 style files

---

## CASM Binary Structure (SFF2/SMF embedded)

```
CASM chunk (variable size, 3-4 KB in Genos files)
├── CSEG header (4 bytes: "CSEG")
│   └── length (4 bytes big-endian)
├── Sdec (Section Definition)
│   └── length (4 bytes) + section name string
│   └── Contains: "Main A", "Main B", "Fill In AA", "Break", etc.
├── Ctb2 blocks (repeated for each section + track combination)
│   ├── Track name (20 bytes, space-padded)
│   └── Track configuration (27 bytes, see below)
└── ... (padding/alignment bytes)
```

## Track Configuration Format (27 bytes per Ctb2)

```
Byte 0-1: Note range low/high (0x00 0x7f = all notes)
Byte 2:   NTR (Note Transposition Rule)
Byte 3-4: Reserved / NTT table index
Byte 5:   NTT (Note Transposition Table)
Byte 6:   Note range high (0x7f = 127)
Byte 7:   NTR for other sections
... (pattern repeats for 3 section groups: Main, Fill, Ending)
```

## NTR Values Found

| Value | Name | Tracks Using |
|-------|------|-------------|
| 0 | Root (Root=Root) | — |
| 1 | Fifth (Root+Fifth) | Rhythm2, Chord2, Phrase2 |
| 2 | Chord (Chord-based) | Chord1, Pad |
| 3 | Bass (Scale-based) | Bass |
| 6 | (fixed/rhythm) | Rhythm2 |

## NTT Values Found

| Value | Name | Notes |
|-------|------|-------|
| 0 | Bypass | — |
| 1 | Default | Phrase2, Phrase1 |
| 2 | Guitar-like | Chord2 |
| 4 | Bass | Bass tracks |
| 6 | Percussion | Rhythm2 (drum fix) |
| 7 | SFF2-specific | Chord1, Pad, Phrase1 |

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

## Semantic Mapping to UASF

| CASM Concept | UASF Field | Status |
|-------------|-----------|--------|
| NTR | chord_rules.ntr (future) | ❌ Not in UASF v1 |
| NTT | chord_rules.ntt (future) | ❌ Not in UASF v1 |
| Note range | track.high_key/low_key | ❌ Not in UASF v1 |
| Track role | track.role | ✅ Partial |
| MegaVoice | articulation.profile | ✅ ADR-013 |

## Fidelity Impact

| Missing Feature | Impact | Priority |
|----------------|--------|----------|
| NTR/NTT mapping | Chord behavior wrong on non-root chords | HIGH |
| Note range filtering | Notes played outside instrument range | MEDIUM |
| Velocity mapping | Articulation not triggered | MEDIUM |
| Section detection | No Intro/Fill/Ending detection | HIGH |
