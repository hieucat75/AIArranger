# Importer Golden Corpus Policy

> **Status:** ACTIVE  
> **Effective from:** Gate 4 (2026-06-13)  
> **Applies to:** All importer modules (SFF1, SFF2, future formats)

---

## Purpose

Ground truth for importer regression testing.
Every importer change must pass all golden files before merge.

## Corpus Structure

```
corpus/
├── yamaha/
│   ├── simple_8beat.sty        # Basic 8-beat, 4 sections
│   ├── ballroom.sty            # Ballroom waltz, 6/8 time
│   ├── mega_guitar.sty         # MegaVoice guitar tracks
│   ├── latin_complex.sty       # Latin percussion, complex NTT
│   ├── sf2_variation.sty       # SFF2 with variations
│   ├── one_track.sty           # Minimal: 1 track, 1 section
│   ├── all_sections.sty        # All section types (I1-M4-F4-E3)
│   └── edge_case.sty           # Malformed headers, empty tracks
├── korg/                       # (future)
└── roland/                     # (future)
```

## Metadata Requirements

Every golden file must have a corresponding `.meta.json`:

```json
{
  "path": "corpus/yamaha/simple_8beat.sty",
  "checksum_sha256": "abc123...",
  "source": "user-provided, license verified",
  "format": "SFF1",
  "sections": 4,
  "tracks": 8,
  "expected_parse": "full",
  "expected_structural_fidelity": 0.95,
  "expected_harmonic_fidelity": 0.80,
  "expected_groove_fidelity": 0.85,
  "known_limitations": [
    "MegaVoice guitar not mapped (degraded GM fallback)"
  ],
  "license_verified": true,
  "added_date": "2026-06-13"
}
```

## License Compliance

- ALL golden files must be **user-owned** or **public domain**
- NO files from commercial style packs without explicit license
- NO files from Yamaha/Korg/Roland official expansion packs
- Each file must have a corresponding license note

## Regression Rules

1. Every PR that changes parsing must update ALL golden file tests
2. Parse success for existing golden files must not decrease
3. Structural fidelity must not decrease by more than 0.05
4. New golden files must be added for new importer features
