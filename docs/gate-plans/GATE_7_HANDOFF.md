# GATE 7 — CASM Semantic Analysis — CLOSE-OUT

> **Status:** ✅ COMPLETE (branch `gate-7-casm-semantic-analysis`, not merged)
> **Baseline:** Gate 6 = 140 assertions → Gate 7 = **173 assertions**, 11/11
> test programs pass via `ctest` / `scripts/run_tests.sh`.

---

## Summary

CASM semantic data from real Yamaha Genos styles is now parsed, grouped
into arranger sections, mapped to UASF roles, tested, and scored. The
critical correctness work was fixing the Ctb2 binary field offsets, which
were wrong in the inherited partial implementation.

## Commits

| Hash | Title |
|------|-------|
| `f1cc463` | G7-FIX: repair CMake build harness (per-test executables + wiring) |
| `cff05a0` | G7-TASK-B: fix CASM Ctb2 field offsets (NTR/NTT, channels, name) |
| `5263cfe` | G7-TASK-C: group CASM track configs into sections via Sdec |
| `5908951` | G7-TASK-D: map CASM tracks to UASF roles + sections |
| `37872db` | G7-TASK-E: tests for CASM parser, Sdec sections, NTR/NTT mapper |
| `b744774` | G7-TASK-F: factor CASM into compatibility score |

## Files changed

- `CMakeLists.txt` — per-test executables, CTest, importer-sff1 lib,
  sff1-parser tool, uasf glob, `CORPUS_DIR` define.
- `scripts/run_tests.sh` — cmake build + ctest runner (new).
- `src/importers/sff1/sff1_types.h` — CasmTrackConfig (src/dst channel,
  ntt_bass), CasmSection, ParseResult::casm_sections.
- `src/importers/sff1/sff1_reader.{h,cpp}` — correct Ctb2 offsets, Sdec
  section grouping.
- `src/importers/sff1/sff1_mapper.{h,cpp}` — mapCasmTrackRole,
  mapCasmSectionType, casm_sections output, CASM-only guard relax.
- `src/importers/sff1/sff1_report.{h,cpp}` — harmonic_fidelity + CASM
  counters + report listing.
- `tests/engine/test_sff1_reader.cpp` — 15 → 48 assertions.
- `docs/importers/casm-structure-notes.md` — validated layout.
- Removed `tools/build_casm.py` (throwaway prompt scaffolding).

## Validated Ctb2 layout (4 Genos files)

```
[0] src channel  [1..8] name  [9] dst channel
[20] note-low  [21] note-high  [22] NTR  [23] NTT (bit7=bass)
```

Real-corpus results:

| Style | CASM sections | tracks | bass | harmonic fidelity |
|-------|--------------:|-------:|-----:|------------------:|
| CLASSIC_6_8 | 9 | 55 | 9 | 83.6% |
| C_WHISPER | 11 | 62 | 17 | 71.0% |
| BALLAD_FOLK | 11 | 67 | 19 | 70.1% |
| POP_ACOUSTIC_2 | 11 | 79 | 10 | 54.4% |

## Key decisions

1. **Role inference is name-primary.** The Ctb2 track name (Bass, Chord1,
   Pad, Phrase1, Rhythm2…) is the most reliable role signal. NTR/NTT
   refine only via the NTT bass-note flag (→ Bass). This avoids
   overclaiming a mapping from transposition rules to instrument roles.
2. **NTR/NTT are not silently dropped.** UASF v1 has no field for them;
   they are recorded in `unmapped_features` (architecture rule #5).
3. **No event splitting.** MIDI events remain in the single SMF MTrk;
   CASM provides section/track *structure* only. Splitting events across
   sections is Gate 8+ (matches the corpus `known_limitations`).
4. **Build harness rebuilt, not patched.** The committed `run-tests`
   target could never link (10 `main()`s) and omitted the uasf sources;
   prior gates were verified with ad-hoc `clang++`. Replaced with
   per-test executables + CTest so the 140→173 baseline is reproducible.
5. **NTT enum mismatch left as-is.** The on-disk NTT indices (0/1/2) do
   not match the placeholder `NttType` enum; reconciling it is Gate 8
   work (full NTT table — an explicit Gate 7 non-goal).

## Risks / limitations

- Ctb2 name field is 8 bytes; longer Yamaha names truncate (e.g.
  "E.P  rt", "Part14"). Cosmetic only.
- `mapCasmSectionType` maps Fill In AA/BB/CC/DD → Fill1-4 and Main A-D →
  Main1-4 by trailing letter; uncommon labels fall back to Main1.
- Harmonic-fidelity heuristic (non-bypass NTT or bass flag) is a coverage
  proxy, not a playback-accuracy guarantee — the Gate 7 "musically wrong
  ≠ compatible" rule still requires the Gate 8 playback/Musical Review.
- Gate 7 plan items T04 (real CoreAudio playback) and T05 (Musical
  Review scorecard) are **not** addressed here — this branch delivers the
  parser/semantic half (T01-T03, T06-T07). See "What's next".

## What's next (Gate 8 candidates)

- Split SMF events into per-CASM-section UASF tracks (by channel + timing).
- Real playback test (UASF → CoreAudio → external MIDI) + Musical Review.
- Full NTT transposition-table implementation; reconcile `NttType` enum.
- Note-range filtering and MegaVoice articulation from CASM/velocity.
