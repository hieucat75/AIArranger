# Gate 10A — Handoff (NTT + Articulation + CASM Hardening)

> Branch `gate-10-ntt-articulation-casm-hardening` off `main` (`5341263`),
> merged via **PR #8** (`79e9f17`), tag **`v0.10.0-gate10`**.
> **Status: ✅ Gate 10A Engine Integration PASS** — 19 test binaries, 292
> assertions, 0 failures. This is the **engine-integration slice only**; it is
> NOT "Gate 10 FULL PASS". Musical / hardware validation is **Gate 10B**.
> Yamaha hardware validation explicitly deferred to **Gate 10B / pre-beta** per
> user spec. No hardware was required for Gate 10A.

---

## 1. Scope (as specified)

Gate 10 did NOT require Yamaha hardware. It focused on:

| Workstream | Status |
|---|---|
| A — Per-channel / per-section splitting (fix PR #7 fidelity drop) | ✅ |
| B — Full NTT tables | ✅ |
| C — Articulation render strategy | ✅ |
| D — CASM→UASF semantic hardening | ✅ |
| E — Tests & handoff | ✅ |

---

## 2. Commits

| Hash | Title |
|---|---|
| `ed4a4c6` | G10-TASK-A: split mixed SMF MTrk into per-channel/per-role UASF tracks |
| `5edb0bd` | G10-TASK-B: full NTR/NTT transposition tables (engine/music/ntt) |
| `e54f2f8` | G10-TASK-C: articulation render strategy (IArticulationRenderer) |
| `92a6a15` | G10-TASK-D: harden CASM→UASF mapping against malformed metadata |

(+ this handoff commit for Task E.)

---

## 3. What changed

### Task A — Per-channel / per-role splitting
- **Problem (PR #7):** the SFF1 reader collapses the whole style into a single
  mixed-channel `Phrase1` MTrk. Chord transpose then hit drums too, and the
  per-channel NoteOn dedupe collapsed unrelated voices — ~30% of retriggers
  dropped at dispatch (4876 source note events → 3384 dispatched).
- **Fix:** `sff1_mapper` now buckets the SMF stream by MIDI channel and emits
  one UASF track per channel, resolving each channel's role from the CASM Ctb2
  metadata (`source_channel → name → role`), **not** a channel heuristic —
  drums are not always on GM channel 9 (POP_ACOUSTIC_2 puts Rhythm1 on ch 8).
- **Result on POP_ACOUSTIC_2:** 1 track → **9 per-role tracks** (Drum,
  Percussion, Bass, Chord×2, Accompaniment…). Dispatched recovered 3384 → 3708.

### Task B — Full NTR/NTT tables (`src/engine/music/ntt.{h,cpp}`)
- Replaces the crude transpose (Major→root+4/+7, Minor→root+3/+7) that
  collapsed every melodic voice onto two pitch classes.
- Implements the chord transform from `arranger-engine-spec-v0.9.md §6`,
  **vendor-agnostic** and **realtime-safe** (stack-only, no alloc/lock/syscall):
  - **NTR:** RootTransposition / RootFixed / Bypass (Guitar + Fifth fall back to
    RootTransposition for v1, with intent preserved via role).
  - **NTT:** Bypass / Melody / Chord / Bass + scale modes (Melodic, Harmonic,
    Natural minor, Dorian). Source chord tone → target chord tone by degree;
    in-scale tension preserved; non-scale tones snap to the nearest chord/scale
    tone (signed, so a maj7 over a triad resolves *up* to the root).
- `StylePlayer` derives the `(NTR, NTT)` pair from **track role** (which
  round-trips through UASF; the raw CASM integers do not), so the transform
  stays correct for a deserialised style. Drums → Bypass.
- **Result:** dispatched 3708 → **3876** (combined A+B: 3384 → 3876, +15%).

### Task C — Articulation render strategy (`src/engine/articulation/renderer.{h,cpp}`)
- `IArticulationRenderer::render(event, articulation, EventSink)` — the seam
  where articulation rendering plugs into dispatch. Realtime contract: no
  alloc/lock; output pushed through a caller-owned sink.
- **NaiveRenderer** — pass-through baseline (default; byte-for-byte unchanged
  playback).
- **KeyswitchRenderer** — for a keyswitch-tagged NoteOn, injects a brief
  articulation-select note on the keyswitch key strictly before the main note;
  guards against re-triggering its own key and tick-0 underflow.
- `StylePlayer::setArticulationRenderer()` selects the strategy; default stays
  Naive.

### Task D — CASM→UASF semantic hardening (`sff1_mapper`)
Every edge case now has a defined contract:
| Edge case | Behaviour |
|---|---|
| Track with no CASM metadata | Fallback role (ch9=Drum, else melodic Phrase1) + one-shot warning per channel |
| Missing NTR/NTT (zeros) | Role from track name; zero NTR/NTT carried verbatim, never invented |
| Abnormal config byte (source_channel > 15) | Dropped + logged; valid channels still map |
| Bass-on / bass-off flag | Bass flag promotes to Bass even w/o keyword; bass-off + no keyword stays non-Bass |
| Sdec section overlap (duplicate names) | Both sections preserved (no merge) |
| Empty parse result | Fail-fast with error |

---

## 4. Test evidence

| Suite (new in Gate 10) | Assertions |
|---|---|
| `test_per_channel_split` (corpus-gated) | 11 |
| `test_ntt` | 19 |
| `test_articulation_renderer` | 15 |
| `test_casm_uasf_mapping_edge_cases` | 16 |
| **Gate 10 new total** | **61** |

- Full suite: **19 binaries, 292 assertions, 0 failures** (`ctest`).
- Baseline (15 binaries / pre-Gate-10) remains green; corpus roundtrip
  (`test_sty_to_uasf_roundtrip`) unchanged at notes=4876, res=1920, channels=9.
- `test_per_channel_split` soft-skips without the proprietary corpus, so CI
  without styles still passes.

---

## 5. Decisions worth noting

1. **Role-based NTR/NTT selection (not raw CASM integers).** The UASF binary
   format does not serialise the CASM `ntr`/`ntt` bytes, so a deserialised
   style would lose them. Track **role** *is* serialised and already encodes the
   musical intent (Bass→Bass NTT, Chord→Chord NTT, drums→Bypass), and Task A
   assigns role from CASM at import time. This keeps the transform correct
   end-to-end **without a format change** — preserving the committed `.uasf`
   corpus and the `korg-playback` tool. The NTT engine still accepts explicit
   `(NTR, NTT)` for callers that have them (unit-tested directly).
2. **Dispatch recovery stops at 3876, not 4876 — and that is correct.** No file
   data is lost (`roundtrip_notes` stays 4876). The residual gap is the
   per-channel NoteOn dedupe correctly merging genuinely overlapping voices that
   chord transposition maps onto the same MIDI note at the same time; removing
   it would reintroduce stuck-note risk (the very thing PR #7's dedupe fixed).
3. **No UASF format change.** Avoided to keep on-disk `.uasf` corpus + tools
   loadable; revisit only if per-track CASM transposition must survive roundtrip
   independently of role.
4. **Realtime safety preserved.** `ntt::transpose` and the renderer path are
   stack-only, allocation-free, lock-free — safe on the dispatch/audio path.

---

## 6. Known Non-Bugs (intentional, NOT data loss)

- **3876 dispatched vs 4876 source note events.** The gap is intentional dedupe
  of duplicate voices produced by chord transpose: when two source voices map
  onto the same MIDI note at the same time, the second NoteOn (and its orphan
  NoteOff) is suppressed to keep one balanced NoteOn/NoteOff pair per sounding
  note. This avoids the stuck-note risk PR #7's dedupe was added to fix. Source
  file integrity is verified independently by `roundtrip_notes = 4876` (no
  events are lost on disk). **This is correct MIDI hygiene, not data loss.**

## 7. Residual risks / open items (→ Gate 10B)

> These define **Gate 10B (pre-beta dependency)**. Gate 10A does NOT cover them.

1. **Yamaha/Korg hardware playback validation** — manual scorecard on a real
   device (also covers G9 P1 #1, #2, still pending hardware).
2. **Swing / shuffle groove model** — deferred from G9 P1 #3; not modeled.
3. **Intro one-bar-delay semantics** — deferred from G9 P1 #5; decide keep vs
   immediate-start.
4. **NTT A/B calibration against authentic Yamaha audio** — the NTT engine is
   implemented but NOT musically validated. Tables are built from music theory
   (spec §6.3), "sounds right" not "bit-identical to Yamaha"; tune by ear vs
   reference.
5. **Guitar (SFF2) NTT + true MegaVoice rendering** — currently fall back to
   RootTransposition / Naive.
6. **Per-*section* event splitting** (vs per-channel) — SMF marker-based section
   boundaries are not yet parsed.

---

## 8. Merge status

**MERGED** — PR #8 approved by PTH and merged to `main` (merge SHA `79e9f17`),
tagged **`v0.10.0-gate10`**. Post-merge suite on `main`: 19 binaries / 292
assertions / 0 failures.

> Gate 10A (engine integration) is closed. **Gate 10B** (hardware + groove + NTT
> calibration, §7) remains the next gate and requires Yamaha/Korg hardware.

### CI note
No GitHub Actions CI is configured for this repo (`.github/workflows/` absent),
so there are no PR checks to report green/red. Verification is the local CTest
suite (292/292, 0 fail), reproduced post-merge on `main`. Standing up CI is an
infra follow-up, not a Gate 10A blocker.
