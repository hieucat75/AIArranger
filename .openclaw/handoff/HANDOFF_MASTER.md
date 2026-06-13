# H A N D O F F   P A C K A G E
## AI Arranger / Style Maker — Gate 7 (CASM Semantic Analysis)
## Transfer to: Agent [TBD by PTH]

---

## 1. PROJECT SNAPSHOT

### Gates Completed
| Gate | Status | SHA | Tests |
|------|--------|-----|-------|
| G1: Realtime Timing Spike | ✅ Merged | d6e9e8a | 46/46 |
| G2: Minimal Arranger Slice | ✅ Merged | 799ccd0 | 87/87 |
| G3: UASF Format v1 | ✅ Merged | e5fd139 | 125/125 |
| G4: SFF1 Research + Governance | ✅ Merged | fc8f659 | 125/125 |
| G5: SFF1 Parser Spike | ✅ Merged | ebaa59c | 140/140 |
| G6: SFF2/SMF Corpus Validation | ✅ Merged | 72131a5 | 140/140 |
| G7: CASM Semantic Analysis | 🔶 BRANCH | a674abb | 140/140 |

### Current Branch
`gate-7-casm-semantic-analysis` — NOT merged to main yet

### Repository
https://github.com/hieucat75/AIArranger

---

## 2. KEY ARCHITECTURE RULES

1. Runtime must NOT know Yamaha/Korg/Roland — src/engine/ is vendor-agnostic
2. Importer layer is ISOLATED — src/importers/sff1/ is where vendor code lives
3. UASF is the canonical format — runtime only understands UASF types
4. No DRM bypass — encrypted styles are REJECTED, not decrypted
5. No silent fallback — unsupported features are LOGGED + REJECTED
6. No malloc/mutex/fileIO in realtime path
7. No MegaVoice->GM silent mapping

---

## 3. CASM STRUCTURE (FROM 4 REAL GENOS .S718 FILES)

CASM chunk structure:
- "CSEG" (4-byte marker)
- Sub-chunks: "Sdec" (Section Definition), "Ctb2" (Track Configuration Block)
- Ctb2: 47 bytes per entry = track_name[20] + config[27]
- Config byte0=low_key, byte1=high_key, byte2=NTR, byte5=NTT

### NTR Values
0=Root, 1=Fifth, 2=Chord, 3=Melodic, 4=Scale, 5=Bypass, 6=Fixed

### NTT Values
0=Bypass, 1=Default, 2=Guitar, 3=Bass, 4=Scale, 5=Chord, 6=Percussion, 7=SFF2-ext

---

## 4. IMPLEMENTATION TASKS (Not Done)

### Task A — CasmTrackConfig types (DONE)
- struct added to sff1_types.h
- vector added to ParseResult

### Task B — CASM parsing methods (PARTIALLY DONE)
- Declared in sff1_reader.h: parseCasm(), parseCtb2Block()
- Call added in sff1_reader.cpp parseBuffer()
- **IMPLEMENTATIONS MISSING** — parseCasm() and parseCtb2Block() need bodies

### Task C — Section boundary detection (NOT DONE)
- All events in single section currently
- Need CASM Sdec section definitions
- Create separate UASF sections

### Task D — NTR/NTT > UASF mapping (NOT DONE)
- sff1_mapper.cpp needs CASM data
- Determine TrackRole from NTR/NTT

### Task E — Tests (NOT DONE)
- Add CASM-specific tests to test_sff1_reader.cpp

### Task F — Compatibility score update (NOT DONE)
- Add harmonic/arranger metrics to sff1_report.cpp

---

## 5. CORPUS FILES (4 GENOS .S718 STYLES)

1. CLASSIC_6_8_SC_GENOS — 39,850 B, 4,878 events
2. C_WHISPER_SC_GENOS — 35,366 B, 3,539 events
3. BALLAD_FOLK_SC_GENOS_ — 38,702 B, 4,325 events
4. POP_ACOUSTIC_2_SC_GENOS — 44,486 B, 5,563 events
