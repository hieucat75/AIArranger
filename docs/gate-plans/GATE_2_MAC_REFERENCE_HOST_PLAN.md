# Gate 2 — Mac Live-Performance Reference Host (JUCE) — Implementation Plan

> Direction (PTH, 2026-07-11): **Mac-first for engine/integration/verification;
> iPad-first for the final product.** This host is a **reference implementation +
> integration harness** to get the live loop correct and measured on the Mac
> before any iPad UI. NOT the shippable product.

**Goal loop (end-to-end):** real MIDI keyboard → CoreMIDI in → parser/router →
chord → `EngineSession`/`StylePlayer` → CoreMIDI out → external keyboard / sound
module. Driven through the existing `LiveEngineFacade`.

---

## 1. What already exists (reuse, don't rebuild)

- `apps/macos` JUCE app shell (Gate 14/15): `Main` + `MainComponent` + panels +
  `EngineDriver` (runs the engine on a `HighResolutionTimer`, output-only today).
- `LiveEngineFacade` (Phase 2): portable input→chord→player→output loop + snapshot
  + command queues. **The host should drive this facade instead of hand-wiring.**
- `CoreMidiOut` / `CoreMidiOutputProvider` (output) and `CoreMidiIn` /
  `IMidiInputSource` (input) in `platform-apple`.
- SFF1 importer (`importer-sff1`) → `uasf::StyleDefinition`; `StylePlayer.loadStyle`.

## 2. What Gate 2 adds

1. **MIDI INPUT in the host** — the current `EngineDriver` is output-only. Wire a
   `CoreMidiIn` (platform adapter) as the facade's `IMidiInputSource`, so a real
   keyboard drives chords. Input pick UI + status.
2. **Drive the facade, not the raw session** — replace `EngineDriver`'s hand-wired
   session loop with a `LiveEngineFacade` timer pump (`facade.tick(48)` per ~1 ms),
   reading `facade.snapshot()` for the UI. Keeps CoreMIDI in the platform layer.
3. **SFF1 load path** — a "Load Style…" file picker → `Sff1Reader` + `sff1_mapper`
   → `uasf::StyleDefinition` → `session.loadStyle()`. Show style name + section list.
4. **Section/transport controls** — buttons that post `ControlEvent`s via the
   facade: Start/Stop, Sync Start, Intro, Variation A–D, Fill, Break, Ending, plus
   tempo, track mute, and an always-visible Panic.
5. **Device/error status** — input/output connection state + a red banner on
   device loss (transport must NOT stop; DOCTRINE 06).

## 3. Structure / files

- **New (portable, headless-testable):**
  - `src/session/live_engine_facade.*` — extend with `loadStyle(const StyleDefinition&)`
    delegating to `session_.loadStyle`, and section/mute helpers if missing.
  - `src/importers/sff1/…` — add a convenience `loadSff1File(path) -> StyleDefinition`
    (thin wrapper over reader+mapper) if not already present, for the host + tests.
- **New (macOS host, local build only — BUILD_MACOS_APP=ON, needs JUCE fetch):**
  - `apps/macos/Source/LiveHostComponent.{h,cpp}` — the reference-host UI (or extend
    `MainComponent`): input/output pickers, Load Style, chord label, transport +
    section buttons, tempo, mute, panic, status banner.
  - Extend `EngineDriver` (or a new `LiveHostDriver`) to own a `CoreMidiIn` +
    `CoreMidiOutputProvider` and pump a `LiveEngineFacade`.
- **New tests (headless, CI):**
  - `tests/**/test_sff1_load_e2e` — load each corpus `.sty` → StyleDefinition →
    `StylePlayer.loadStyle` → play N ticks → assert events emitted, no crash, no
    stuck notes.
  - Facade section/transport command tests (Start/Stop/Variation/Fill/Ending change
    the sequenced section on bar boundaries).

## 4. Threading / ownership (unchanged contract)

- Facade `tick()` on the JUCE timer thread is the sole producer of the engine's
  control queue; MIDI-in read thread and UI both feed the facade's two SPSC queues.
- CoreMIDI lives only in `platform-apple` (`CoreMidiIn`/`CoreMidiOut`); the host
  passes those as `IMidiInputSource*` / `IMidiOutputProvider*` into the facade.
- No malloc/lock/ObjC in the CoreMIDI read callback (already honoured).

## 5. Acceptance criteria

- **Headless (CI, do now):** SFF1 corpus loads + plays end-to-end (0 crash, no
  stuck notes); facade section/transport commands change sections on bar
  boundaries; latch/sustain/panic behave per Gate 3 tests; all suites green, 0 warn.
- **Local macOS build (PTH, no hardware):** `-DBUILD_MACOS_APP=ON` builds the host;
  UI shows pickers/controls; loads a style; transport runs against a virtual MIDI
  destination (e.g. IAC) with no external gear.
- **Real hardware (PTH, STOP point):** a physical keyboard drives chords and an
  external tone generator plays the accompaniment; chord→out latency measured;
  30–60 min soak with 0 stuck notes / 0 transport drops on hot-plug.

## 6. Test split — headless vs. hardware

| Verifiable headless now (CI) | Needs local macOS build (no HW) | Needs real MIDI hardware (PTH) |
|---|---|---|
| SFF1 load→play e2e; section/transport command logic; latch/sustain/panic; facade snapshot; no-stuck-notes | JUCE host builds + launches; UI wiring; virtual-MIDI (IAC) loopback | Real keyboard in; external sound module out; latency + soak + hot-plug |

## 7. Risks

**P0** — CoreMIDI read-callback RT-safety when wired to the facade (keep the sink
lock-free; already SPSC). Stuck notes on device loss (panic-on-loss + note-balance
asserts). SFF1 load correctness on real corpus (validate against the 4 Genos files).

**P1** — JUCE fetch/build only verifiable locally (network + `BUILD_MACOS_APP=ON`);
the engine wiring is proven headless, the GUI is proven on PTH's Mac. Bar-boundary
transition edge cases (double-ending, mid-fill chord change).

## 8. Sequence (autonomous; stop only at real MIDI hardware)

1. Headless: SFF1 load→play e2e test; chord latch + sustain + panic tests (Gate 3
   correctness) — implement + verify in CI. ← **doing now**
2. Facade: `loadStyle` + section/mute helpers + tests.
3. macOS host: wire `CoreMidiIn` + facade into the driver; add the minimal live UI
   (authored; verified by PTH's local `BUILD_MACOS_APP=ON` build).
4. STOP for hardware: real keyboard + sound module for end-to-end / latency / soak.
