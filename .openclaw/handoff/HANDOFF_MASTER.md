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
| G7: CASM Semantic Analysis | ✅ BRANCH (ready) | b744774 | 173/173 |
| G8: NTR/NTT + Chord-Aware Transpose | ✅ Merged to main | d7441b7 | folded into G9 |
| G9: Multi-Playback Validation | ✅ DONE — CONDITIONAL PASS (manual Korg pending) | edb6a20 | 218/218 (14 binaries, 0 fail) |
| G9.1: .sty→.uasf resolution fix | ✅ Merged | 5341263 (PR #7) | 15 binaries, 0 fail |
| G10: NTT + Articulation + CASM hardening | ✅ DONE (branch, PR open) | (branch `gate-10-ntt-articulation-casm-hardening`) | 292/292 (19 binaries, 0 fail) |

### Current Branch / Release
`main` — Gate 9 merged via PR #6. Merge SHA `edb6a20`, tag **`v0.9.0-gate9`**.

> **Gate 9 → FULL PASS** once PTH completes the manual Korg review (Groove +
> Articulation) and records the scores into `.openclaw/review/GATE_9_MUSICAL_REVIEW.md`.
> See `docs/gate-plans/GATE_9_MANUAL_KORG_CHECKLIST.md` and `GATE_9_OPEN_ITEMS.md`.

> Note: the current committed test sources enumerate **143** baseline (G7+G8) assertions
> in this tree; Gate 9 adds 75 → **218 total** (the earlier "173" figure reflected an older
> enumeration and is unrelated to Gate 9). See `docs/gate-plans/GATE_9_HANDOFF.md`.

### Gate 10 scope (✅ DONE — see `docs/gate-plans/GATE_10_HANDOFF.md`)
- ✅ Per-channel/per-role track splitting (fixes PR #7 musical-fidelity drop)
- ✅ Full NTR/NTT tables (`src/engine/music/ntt`) — Bass/Chord/Melody + scale modes
- ✅ Articulation render strategy (`IArticulationRenderer`: Naive + Keyswitch)
- ✅ CASM→UASF semantic hardening (defensive mapper + edge-case suite)

### Gate 10B scope (next — Yamaha hardware / pre-beta)
- Yamaha hardware playback validation (deferred per user spec)
- Swing/shuffle groove model (G9 P1 #3, still open)
- Intro one-bar delay semantics decision (G9 P1 #5, still open)
- NTT A/B calibration vs reference Yamaha audio (tune tables by ear)
- SFF2 Guitar NTT + true MegaVoice rendering (currently fall back)

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

## 4. IMPLEMENTATION TASKS — ALL DONE (Gate 7 complete)

> See `docs/gate-plans/GATE_7_HANDOFF.md` for the full close-out report.

### Task A — CasmTrackConfig types ✅ DONE
- struct + ParseResult vector (extended in Task B with channels + ntt_bass)

### Task B — CASM parsing methods ✅ DONE
- parseCasm()/parseCtb2Block() had bodies but WRONG field offsets
  (20-byte name, NTT at byte 25 = garbage). Corrected to the validated
  47-byte Ctb2 layout: src-channel[0], name[1..8], NTR[22], NTT[23].

### Task C — Section detection (Sdec) ✅ DONE
- parseCasm groups Ctb2 configs under each Sdec section
- ParseResult::casm_sections (name + tracks); CASM section *structure*
  exposed. Event-to-section splitting deferred to Gate 8.

### Task D — NTR/NTT → UASF mapping ✅ DONE
- sff1_mapper: mapCasmTrackRole (name-primary + NTT bass flag),
  mapCasmSectionType; SffToUasfResult::casm_sections built from CASM.
- NTR/NTT logged in unmapped_features (no UASF v1 field).

### Task E — Tests ✅ DONE
- test_sff1_reader 15 → 48 assertions (synthetic CASM + real corpus +
  mapper). CMake passes CORPUS_DIR.

### Task F — Compatibility score ✅ DONE
- harmonic_fidelity + casm_sections/casm_tracks/bass_tracks in report.

### Build harness (G7-FIX) ✅
- CMake `run-tests` target never built (10 main()s) and omitted the uasf
  glob. Replaced with per-test executables + CTest; added importer-sff1
  lib and sff1-parser tool. Baseline restored to 140, now 173 assertions.

---

## 5. CORPUS FILES (4 GENOS .S718 STYLES)

1. CLASSIC_6_8_SC_GENOS — 39,850 B, 4,878 events
2. C_WHISPER_SC_GENOS — 35,366 B, 3,539 events
3. BALLAD_FOLK_SC_GENOS_ — 38,702 B, 4,325 events
4. POP_ACOUSTIC_2_SC_GENOS — 44,486 B, 5,563 events
