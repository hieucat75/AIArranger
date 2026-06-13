# GATE 3 — Internal Style Format v1 (UASF)

> **Project:** AI Arranger / Style Maker  
> **Status:** 🔨 IN PROGRESS  
> **Date:** 2026-06-13  
> **Prerequisite:** Gate 2 ✅ (arranger vertical slice)

## Objective

Design and implement UASF (Universal Arranger Style Format) v1:
binary serializer, deserializer, schema validation, roundtrip tests.

## Scope

- UASF binary format specification implementation
- Serializer: write StyleDefinition → binary blob
- Deserializer: read binary blob → StyleDefinition
- Articulation metadata + fidelity flags in serialized format
- File-based style loading (from .uasf files)
- Validation tool: verify schema correctness
- Roundtrip test: serialize → deserialize → compare

## Non-Goals

- Yamaha SFF1/SFF2 import (Gate 4)
- Compression or encryption
- Streaming or partial loading
- Version migration beyond v0.1

## Files

| Task | Files | Description |
|------|-------|-------------|
| G3-T01 | `src/engine/uasf/format.h/.cpp` | Binary format constants, header struct, section layout |
| G3-T02 | `src/engine/uasf/serializer.h/.cpp` | StyleDefinition → binary writer |
| G3-T03 | `src/engine/uasf/deserializer.h/.cpp` | Binary reader → StyleDefinition |
| G3-T04 | `src/engine/uasf/validator.h/.cpp` | Schema validation + integrity check |
| G3-T05 | Update `src/engine/arranger/style_player.cpp` | File-based style loading from .uasf |
| G3-T06 | Update `CMakeLists.txt` | Add new source files |
| G3-T07 | `tests/engine/test_serializer.cpp` | Write/read/compare roundtrip tests |
| G3-T08 | `tests/engine/test_deserializer.cpp` | Malformed input rejection tests |
| G3-T09 | `tests/engine/test_validator.cpp` | Schema validation tests |

## UASF Binary Format (v0.1)

```
┌─────────────────────────┐
│ Magic: "UASF" (4 bytes) │
│ Version: uint32 (1)     │
│ Flags: uint32           │
│ Section count: uint32   │
├─────────────────────────┤
│ Section 1 header        │
│   type: uint8           │
│   bars: uint16          │
│   resolution: uint16    │
│   beats_per_bar: uint8  │
│   beat_note: uint8      │
│   track_count: uint8    │
│ Section 1 track data... │
├─────────────────────────┤
│ Section 2 ...           │
└─────────────────────────┘

Track layout (per section):
  name_length: uint8
  name: char[name_length]
  midi_channel: uint8
  role: uint8
  is_drum: uint8
  articulation profile: uint8
  fidelity: uint8
  event_count: uint32
  events: MidiEvent[event_count]

MidiEvent layout (8 bytes):
  tick_delta: uint32 (relative from previous event)
  type_channel: uint8 (type:4 | channel:4)
  data1: uint8
  data2: uint8
```

## Exit Criteria

- [ ] Serializer produces valid binary .uasf from demo style
- [ ] Deserializer reads binary .uasf back to identical StyleDefinition
- [ ] Roundtrip: serialize(style) → deserialize → match original
- [ ] Articulation metadata preserved through roundtrip
- [ ] Fidelity flags preserved through roundtrip
- [ ] File-based style loading works
- [ ] Validator catches malformed input
- [ ] All tests pass
- [ ] Build passes on macOS
