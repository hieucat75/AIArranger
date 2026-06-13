# HANDOFF: Gate 7 — Claude Code Implementation Package

## Project: AI Arranger / Style Maker
## Branch: gate-7-casm-semantic-analysis
## Current SHA: a674abb

---

## 1. Context

We have built:
- **Gate 1-3:** Realtime engine, MIDI scheduler, UASF format
- **Gate 4:** Yamaha SFF1 research + governance policies
- **Gate 5:** SFF1 binary parser (chunk-based + SMF/SFF2) + CLI tool
- **Gate 6:** Corpus validation with 4 real Genos .S718 files (100/100/100 parse scores)
- **Gate 7 (current):** CASM semantic analysis + structure documentation

Current docs on branch:
- `docs/importers/casm-structure-notes.md` — full CASM binary structure
- `docs/gate-plans/GATE_7_CASM_SEMANTIC_ANALYSIS.md` — plan & exit criteria
- `docs/importers/sff1-structure-notes.md` — SFF format overview

## 2. Critical Discovery: Genos SFF2 Format

Genos .S718 files are NOT chunk-based SFF1. They are **SMF wrappers**:
```
MThd (MIDI header) → 6 bytes
MTrk (MIDI track) → 15-24 KB MIDI events
CASM (config data) → 3-5 KB track configurations
OTSc (OTS Custom) → 16 KB Yamaha hardware-specific
```

The current parser (`src/importers/sff1/`) handles:
- ✅ Chunk-based SFF1 format
- ✅ SMF-based SFF2 format (MThd + MTrk detection)
- ✅ MIDI event extraction from MTrk
- ✅ CASM chunk detection
- ❌ CASM **semantic parsing** (NOT implemented)

## 3. CASM Structure (Decoded)

CASM chunk contains repeating `Ctb2` blocks:
- Each block = 47 bytes
- Track name (20 bytes, space-padded)
- Track config (27 bytes):
  - Byte 0-1: Note range low/high (0x00 0x7f = all)
  - Byte 2: NTR (Note Transposition Rule)
  - Byte 3-4: Reserved
  - Byte 5: NTT (Note Transposition Table)  
  - Byte 6: Note range high (0x7f)
  - Byte 7: NTR alternative
  - Pattern repeats for section variations

NTR values found:
- 0 = Root
- 1 = Fifth  
- 2 = Chord
- 3 = Bass
- 6 = Fixed/Rhythm

NTT values found:
- 0 = Bypass
- 1 = Default
- 2 = Guitar
- 4 = Bass
- 6 = Percussion
- 7 = SFF2-specific

Track roles in CASM: Rhythm2, Bass, Chord1, Chord2, Pad, Phrase1, Phrase2

## 4. Current P1 Blockers (Codex-identified)

1. **NTR/NTT not mapped to UASF** — Chord behavior will be wrong on non-root harmonies
2. **Section boundaries not parsed** — All events merged into single "SMF (SFF2/Genos)" section

## 5. Implementation Tasks for Claude Code

These are the coding tasks. Do NOT research further. Implementation only.

### TASK A: CASM Parser (src/importers/sff1/sff1_reader.h/.cpp)

Add CASM semantic parsing:
- Parse `CSEG` → extract section definitions
- Parse `Sdec` → section names ("Main A", "Fill In AA", etc.)
- Parse `Ctb2` blocks → extract NTR, NTT, note range, track role
- Map parsed CASM to existing `ParseResult` data structures

Files to modify: `src/importers/sff1/sff1_reader.h/.cpp` (existing)

### TASK B: CASM Data Types (src/importers/sff1/sff1_types.h)

Add:
```cpp
struct CasmTrackConfig {
    uint8_t high_key, low_key;     // Note range
    NtrType ntr;                   // Note Transposition Rule
    NttType ntt;                   // Note Transposition Table
    SffTrackRole role;             // Track role
    bool is_megavoice_candidate;
};
```

Files to modify: `src/importers/sff1/sff1_types.h` (existing)

### TASK C: Section Boundary Detection

Based on CASM section definitions:
- Create separate UASF sections for each detected section type
- Route MIDI events to the correct section
- Support: Intro1-3, Main1-4, Fill1-4, Ending1-3, Break

### TASK D: NTR/NTT → UASF Mapping

In `src/importers/sff1/sff1_mapper.cpp`:
- Map NTR to a chord rule type in UASF
- Map NTT to articulation profile
- Preserve note range for articulation metadata

### TASK E: Tests

- Parse CASM data from all 4 corpus files
- Verify NTR/NTT values extracted correctly
- Verify section count matches CASM Sdec definitions

### TASK F: Compatibility Score Update

- Add harmonic fidelity metric based on NTR/NTT mapping
- Add arranger behavior fidelity (section boundary detection success)

Supporting file: `docs/importers/casm-structure-notes.md` (reference doc)

---

## 6. Non-Goals (for this Gate)

- Do NOT implement MegaVoice rendering (deferred)
- Do NOT add UASF binary field changes (requires MAJOR version bump)
- Do NOT attempt full NTT table implementation (complex, scope creep)
- Do NOT modify runtime engine (importer layer only)

## 7. Exit Criteria (Gate 7)

- [ ] CASM data parsed from corpus files → NTR/NTT extracted
- [ ] Section boundaries detected from CASM Sdec
- [ ] UASF conversion creates separate sections per section type
- [ ] NTR/NTT values mapped to UASF articulation metadata
- [ ] Tests pass (existing 140+ + new CASM tests)
- [ ] Codex review pass
- [ ] Musical review template prepared

## 8. Files Overview

```
src/importers/sff1/
├── sff1_types.h       → ADD CasmTrackConfig, update SffSection
├── sff1_reader.h/.cpp → ADD CASM parsing methods
├── sff1_mapper.h/.cpp → UPDATE NTR/NTT → UASF mapping
├── sff1_report.h/.cpp → UPDATE compatibility score

corpus/yamaha/sff1/   → 4 Genos .sty files + meta.json + .uasf

tests/engine/
└── test_sff1_reader.cpp → ADD CASM tests

docs/importers/
├── casm-structure-notes.md     → Reference
├── sff1-importer-architecture.md → Architecture
└── sff1-unsupported-features.md → Limitations

docs/gate-plans/
└── GATE_7_CASM_SEMANTIC_ANALYSIS.md → Plan
```
