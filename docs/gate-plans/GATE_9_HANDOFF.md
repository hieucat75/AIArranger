# Gate 9 — Multi-Playback Validation & Musical Feel — CLOSE-OUT

> **Branch:** `gate-9-multi-playback-validation` (forked from Gate 7, then merged Gate 8)
> **Date:** 2026-06-14
> **Status:** ✅ Complete — engine-verifiable validation done; manual Korg pass documented & pending hardware.

---

## 0. Evidence

| Item | Value |
|------|-------|
| PR | https://github.com/hieucat75/AIArranger/pull/6 |
| Merge into `main` (SHA) | `edb6a20ce2a19c125db72566349c737f11648540` |
| Tag | `v0.9.0-gate9` (→ `edb6a20`) |
| Tests | **218 assertions / 14 binaries / 0 fail** |
| Latency (schedule→dispatch, dev Mac) | p50 ≈ 200 µs, **p99 ≈ 385 µs**, no loss |
| Musical Review | **17 / 25 — CONDITIONAL PASS** (`.openclaw/review/GATE_9_MUSICAL_REVIEW.md`) |
| Open P1 items | see `docs/gate-plans/GATE_9_OPEN_ITEMS.md` |
| Manual Korg checklist | see `docs/gate-plans/GATE_9_MANUAL_KORG_CHECKLIST.md` |

> Gate 9 reaches **FULL PASS** once PTH completes the manual Korg review (Groove +
> Articulation) and records the scores into the Musical Review.

---

## 1. What Gate 9 set out to do

Validate the playback pipeline end-to-end and add the real external-MIDI path, so a
human can finally hear a style on a Korg/Yamaha arranger. Gate 9 was branched from Gate 7
**without** a standalone Gate 8 branch in the line; per PTH's decision, Gate 8
(`gate-8-ntr-ntt-and-external-playback`) was **merged into the Gate 9 base first**, then
Gate 9 implemented on top.

---

## 2. Commits (in order)

| SHA | Title |
|-----|-------|
| `239a585` | merge: pull Gate 8 NTR/NTT + chord-aware transpose into Gate 9 base |
| `4977f78` | G9-TASK-A: CoreMIDI external output — lock-free SPSC queue + dispatch thread |
| `43df79e` | G9-TASK-B: playback validation harness + correct StylePlayer dispatch |
| `b83b466` | G9-TASK-C: NoteBalance stuck-note / orphan detector |
| `34a9598` | G9-TASK-D: latency + MIDI stability harness |
| `f72e836` | G9-TASK-E: Korg playback CLI for manual Musical Review |
| `(this)`  | G9-DONE: close Gate 9 — review + handoff docs |

---

## 3. Files changed

### New
- `src/engine/midi/coremidi_out.{h,cpp}` — real CoreMIDI output (Task A)
- `src/engine/midi/note_balance.{h,cpp}` — stuck-note / orphan detector (Task C)
- `tests/engine/test_coremidi_out.cpp` — 22 assertions (Task A)
- `tests/engine/test_playback_validation.cpp` — 23 assertions + `MidiCaptureSink` (Task B)
- `tests/engine/test_latency_stability.cpp` — 9 assertions (Task D)
- `tools/korg_playback_cli.cpp` — manual playback driver (Task E)
- `.openclaw/review/GATE_9_MUSICAL_REVIEW.md` — scorecard (Task F)
- `docs/gate-plans/GATE_9_HANDOFF.md` — this file

### Modified
- `src/engine/arranger/style_player.{h,cpp}` — **dispatch rewrite** (Task B): monotonic
  per-section cursor; active-note tracking + flush on switch/stop/chord-change; NoteOn/NoteOff
  dedupe against the active set.
- `src/engine/midi/panic.{h,cpp}` — `flushActiveNotes()` (per-note NoteOff) + `isNoteActive()`.
- `tests/engine/test_panic.cpp` — 12 → 33 assertions (NoteBalance + new PanicHandler methods).
- `CMakeLists.txt` — link CoreMIDI + CoreFoundation; add `korg-playback` target.
- (from G8 merge) `style_player.cpp`, `uasf/types.h`, `sff1/sff1_mapper.cpp` — NTR/NTT.

---

## 4. Key decisions & root-cause fixes

1. **G7 ↔ G8 merge was conflict-free.** Gate 9 (`ba035de`) only added a plan doc on top of
   `f865376`; Gate 8 touched disjoint files (`style_player.cpp`, `types.h`, `sff1_mapper.cpp`
   + docs). Merged with `--no-ff`, no resolution needed. G7 tests kept; G8 transpose logic kept.

2. **The Gate-2 dispatch was a stub and was unsafe — fixed.** `dispatchSectionEvents` abused
   the *track index* as event progress, so each section dispatched **once** and left stuck
   notes (probe: 16 NoteOn / 14 NoteOff). Replaced with a **monotonic per-section cursor**:
   each event whose section-relative tick falls in `(cursor, rel]` fires exactly once as time
   advances. A full intro→main→fill→main→ending cycle is now **balanced (250/250, 0 stuck)**.

3. **Stuck notes across transitions — fixed with flush.** StylePlayer now tracks dispatched
   (post-transpose) notes via `PanicHandler` and calls `flushActiveNotes()` on section switch,
   stop, and chord change. `flushActiveNotes` emits one NoteOff per active note (so a
   `NoteBalance` observer sees balanced pairs) — unlike `panic()` which sends CC120/123.

4. **Voicing collision — fixed with dedupe.** G8's chord-tone transpose can map two source
   voices onto the **same** MIDI note (e.g. C-major chord {60,64,67} → {64,67,64}). The
   single-bit active tracker then under-counted, producing one stuck note on flush. Dispatch
   now **dedupes**: skip a NoteOn if the note is already sounding, skip a NoteOff if it is not
   — correct MIDI hygiene and keeps the bit-tracker consistent.

5. **Realtime safety (ADR-013) held.** The audio/producer path (`send()`,
   `scheduler.scheduleEvent`) only touches a lock-free SPSC queue — no malloc/mutex/syscall/
   MIDISend. A **dedicated dispatch thread** does `MIDIPacketList` + `MIDISend`. Hotplug is
   handled by `MIDINotifyProc` plus a periodic re-resolve-by-name in the dispatch thread (so
   reconnect works even with no CoreMIDI run loop, e.g. in tests). Control-thread/dispatch-thread
   name access is guarded by a mutex that the RT path never touches.

6. **CI safety.** With 0 destinations everything degrades to a graceful no-op (counted as
   dropped). `midiEventToBytes()` is a pure function and fully unit-tested without hardware.

---

## 5. Test count

All **14** test binaries green, **0 failures**.

| Suite | Assertions | Note |
|-------|-----------:|------|
| Baseline (G7 + G8, 11 binaries) | 143 | unchanged by Gate 9 (test_panic was 12) |
| Gate 9 — test_coremidi_out | 22 | new |
| Gate 9 — test_playback_validation | 23 | new |
| Gate 9 — test_latency_stability | 9 | new |
| Gate 9 — test_panic delta | +21 | 12 → 33 |
| **TOTAL** | **218** | **all pass** |

> Note on the "173" figure in `HANDOFF_MASTER`: the committed test sources in this tree
> enumerate **143** baseline assertions (the historical 173 reflected an earlier test
> enumeration). This is unrelated to Gate 9 — no SFF1/CASM test code was touched here.

Latency (dev Mac, headless): schedule→dispatch **p50 ≈ 200 µs, p99 ≈ 385 µs**, no loss.

---

## 6. Risks / limitations

- **Swing/shuffle/micro-groove not modelled** — timing is mechanically exact only.
- **Articulation rendering (keyswitch/MegaVoice) not implemented** — metadata only.
- **Full NTT tables simplified** — only root/fifth chord-tone voicing.
- **1-bar startup latency** — a queued intro activates on the *first* bar boundary (sequencer
  semantics, asserted by `test_section_sequencer`); intentionally left unchanged.
- **Demo data inconsistency** — the demo "Fill" declares `bars=1` but its track contains 4
  bars of events; harness validates the *declared* length and flush keeps it balanced.
- **Korg manual pass not yet run** — Groove & Articulation final confirmation needs hardware
  (checklist in the Musical Review).

---

## 7. What's next — Gate 10 (Yamaha)

- Load a real Yamaha `.S718`/SFF1 style through the CASM mapper into UASF and play it through
  `korg-playback` on hardware (closes the manual Musical Review).
- Implement full NTT table application (Guitar/Bass/Scale/Chord/Percussion tables).
- Articulation rendering: keyswitch + MegaVoice handling.
- Consider modelling groove/swing for feel parity.
