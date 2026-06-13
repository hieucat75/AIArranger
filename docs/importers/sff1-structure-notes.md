# SFF1 Structure Notes (Preliminary)

> **Status:** RESEARCH — preliminary analysis  
> **Branch:** gate-4-yamaha-sff1-research  
> **Sources:** Public documentation, binary analysis

---

## File Format Overview

Yamaha SFF1 (Style File Format 1) is a MIDI-based format with custom CASM (Configuration and Setup data Messages) section.

## Top-Level Structure

```
SFF1 File (CASM + MIDI)
├── CASM Header
├── OTS Blocks (4 slots)
├── Section Data
│   ├── Intro 1-3
│   ├── Main 1-4
│   ├── Fill 1-4
│   ├── Ending 1-3
│   └── Break
└── MIDI Data (per section, per track)
    ├── MultiPad (if present)
    ├── Chord 1-2 (if present)
    ├── Bass
    ├── Drum
    ├── Percussion
    ├── Phrase 1-2
    └── Extra
```

## Section Structure

Each style section contains:
- NTR (Note Transposition Rule): Root, Root+5th, Chord, Scale, Fixed
- NTT (Note Transposition Table): Melodic, Bypass, Guitar, etc.
- High Key / Low Key note range
- Original Key
- MultiPad assignment

## CASM Section

Configuration data including:
- Section name/type mapping
- MIDI channel assignments
- DSP/reverb settings (not imported)
- MegaVoice articulation mapping (critical for fidelity)

## Known Challenges

1. **CASM parsing** — Proprietary encoding, partially documented
2. **NTT tables** — Undocumented binary tables; behavior must be reverse-engineered
3. **MegaVoice** — Articulation via velocity ranges + MIDI notes; no standard mapping
4. **DSP effects** — Proprietary Yamaha DSP parameters (not importable)
5. **OTS** — Voice selection for Yamaha sound engines (not applicable to external playback)

## UASF Mapping Plan

| SFF1 Concept | UASF Equivalent | Status |
|-------------|----------------|--------|
| Section types | SectionType enum | ✅ Done |
| MIDI tracks | TrackDefinition | ✅ Done |
| MIDI events | MidiEvent | ✅ Done |
| NTR/NTT | Chord rules (future UASF extension) | ❌ Need research |
| MegaVoice | ArticulationProfile::MegaLike | ✅ ADR-013 |
| DSP | Not mapped | ❌ Out of scope |
| OTS | Not mapped | ❌ External device only |
