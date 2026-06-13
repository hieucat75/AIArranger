# Gate 6 — Real SFF1/SFF2 Corpus Validation

> **Status:** IN PROGRESS  
> **Branch:** gate-6-sff1-corpus-validation  
> **PR:** TBD

---

## Corpus Files (3 real Yamaha Genos styles)

| File | Size | Format | Source |
|------|------|--------|--------|
| `CLASSIC_6_8_SC_GENOS.S718.sty` | 39,850 B | SFF2/SMF | Genos |
| `C_WHISPER_SC_GENOS.S718.sty` | 35,366 B | SFF2/SMF | Genos |
| `BALLAD_FOLK_SC_GENOS_.S718.sty` | 38,702 B | SFF2/SMF | Genos |

## Discovered Structure (SFF2/SMF format)

All 3 files share identical top-level structure:

| Chunk | Size (varies) | Status |
|-------|---------------|--------|
| `MThd` | 6 B (MIDI header) | ✅ Detected |
| `MTrk` | 15-21 KB | ❌ Not parsed (SMF track) |
| `CASM` | 3-4 KB | ✅ Detected |
| `OTSc` | 15-16 KB | ❌ Not parsed (Yamaha OTS Custom) |

## Required Updates

The parser must support **two formats**:

### Format 1: SFF1 (CASM chunk-based)
- Existing implementation
- `SInt`, `Sect`, `Trk` chunks

### Format 2: SFF2/SMF (MThd + MTrk wrapper)
- Parse `MThd` header (format, tracks, resolution)
- Parse `MTrk` chunks: extract MIDI events from SMF
- Map CASM data to section/track structure
- Ignore `OTSc` (OTS Custom data = Yamaha hardware-specific)

## Current Compatibility Scores

| File | Parse | Structural | Events |
|------|-------|------------|--------|
| CLASSIC_6_8 | 100% | 25% (1/4) | 0% |
| C_WHISPER | 100% | 25% (1/4) | 0% |
| BALLAD_FOLK | 100% | 25% (1/4) | 0% |

## Target Scores (after update)

| File | Parse | Structural | Events |
|------|-------|------------|--------|
| CLASSIC_6_8 | 100% | 75%+ | 90%+ |
| C_WHISPER | 100% | 75%+ | 90%+ |
| BALLAD_FOLK | 100% | 75%+ | 90%+ |
