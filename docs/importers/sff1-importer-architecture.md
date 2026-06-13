# SFF1 Importer Architecture

> **Status:** DRAFT  
> **Branch:** gate-4-yamaha-sff1-research

---

## Pipeline

```
SFF1 File (.sty)
     │
     ▼
┌─────────────────────────┐
│   SFF1 Binary Reader    │  ← Parses raw bytes
│   (vendor-specific)     │     Rejects malformed/encrypted
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│   SFF1 Intermediate     │  ← SFF1-specific data model
│   (NOT UASF)            │     CASM, NTT, OTS preserved here
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│   SFF1 → UASF Mapper    │  ← Maps SFF1 → UASF types
│   (vendor-specific)     │     NTT → chord rules
└─────────┬───────────────┘     MegaVoice → articulation
          │                     Unsupported → logged + rejected
          ▼
┌─────────────────────────┐
│   UASF StyleDefinition  │  ← Runtime format (vendor-agnostic)
│   (runtime only)        │     Used by StylePlayer
└─────────────────────────┘
```

## Layer Separation

| Layer | Language | Location | Vendor-Specific? |
|-------|----------|----------|-----------------|
| SFF1 Binary Reader | C++ | `src/importers/sff1/reader.*` | ✅ Yes |
| SFF1 Intermediate | C++ | `src/importers/sff1/types.h` | ✅ Yes |
| SFF1 → UASF Mapper | C++ | `src/importers/sff1/mapper.*` | ✅ Yes (isolated) |
| UASF Runtime | C++ | `src/engine/` | ❌ No |
| Style Player | C++ | `src/engine/arranger/` | ❌ No |

## File Organization

```
src/importers/
├── sff1/
│   ├── sff1_types.h         # SFF1 intermediate data structures
│   ├── sff1_reader.h/.cpp   # Binary parser
│   ├── sff1_casm.h/.cpp     # CASM section parser
│   ├── sff1_mapper.h/.cpp   # SFF1 → UASF conversion
│   └── sff1_megavoice.h/.cpp # MegaVoice detection + mapping
├── sff2/                    # (future)
└── korg/                    # (future)
```

## Key Design Decisions

1. **Parser lives in src/importers/, NOT src/engine/** — Keeps runtime vendor-agnostic
2. **SFF1 Intermediate format is NOT UASF** — Prevents runtime from depending on SFF1 concepts
3. **Mapper is where vendor logic lives** — This is the only place Yamaha concepts appear
4. **Mapper produces UASF only** — Runtime never sees SFF1 types
5. **Unsupported features are logged and rejected** — No silent fallback
