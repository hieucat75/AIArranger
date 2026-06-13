# Claude Code — Gate 7 Implementation Prompt

## Target: Implement CASM semantic parser + NTR/NTT mapper

Read the handoff document first: `.openclaw/handoff/GATE7_CLAUDE_HANDOFF.md`

### Implementation Order

1. Update `src/importers/sff1/sff1_types.h` — add `CasmTrackConfig`, update `SffSection` to include CASM data
2. Update `src/importers/sff1/sff1_reader.h/.cpp` — add CASM parsing (CSEG, Sdec, Ctb2 blocks)
3. Update `src/importers/sff1/sff1_mapper.h/.cpp` — map NTR/NTT to UASF articulation
4. Update `src/importers/sff1/sff1_report.h/.cpp` — add harmonic fidelity metric
5. Update `tests/engine/test_sff1_reader.cpp` — add CASM parsing tests

### Source of Truth

- CASM binary structure: `docs/importers/casm-structure-notes.md`
- Current parser: `src/importers/sff1/sff1_reader.cpp` (see parseBuffer, parseMThd, parseMTrk methods)
- Current types: `src/importers/sff1/sff1_types.h` (see SffSection, SffChunk, CasmSectionInfo)

### Build

```
clang++ -std=c++20 -I src -c src/importers/sff1/sff1_reader.cpp -o /tmp/r.o
clang++ -std=c++20 -I src -c src/importers/sff1/sff1_report.cpp -o /tmp/rp.o
clang++ -std=c++20 -I src -c src/importers/sff1/sff1_mapper.cpp -o /tmp/m.o
clang++ -std=c++20 -I src -c src/engine/uasf/serializer.cpp -o /tmp/s.o
clang++ -std=c++20 -I src -c tools/sff1_parser_cli.cpp -o /tmp/c.o
clang++ /tmp/s.o /tmp/r.o /tmp/rp.o /tmp/m.o /tmp/c.o -o sff1-parser
```

### Test

```
./sff1-parser corpus/yamaha/sff1/CLASSIC_6_8_SC_GENOS.S718---36428220-aac8-423d-9a87-79f1594df699.sty
```

### Rules

- Importer layer only (`src/importers/sff1/`)
- No Yamaha/Korg logic in runtime (`src/engine/`)
- No DRM bypass
- No copyrighted samples
- Every commit must build
