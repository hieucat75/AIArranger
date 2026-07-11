# iOS Thin Vertical Slice — Implementation Plan

> **⏸ DEFERRED TO GATE 5 (PTH, 2026-07-11 update).** Direction refined to
> **Mac-first engine / iPad-first product**: the engine live loop is proven on the
> macOS reference host (Gates 2–4) *before* any iPad SwiftUI work. This plan is
> retained as the Gate-5 blueprint; the portable-core split + `LiveEngineFacade`
> it prescribed are already landed and feed Gate 4. Do not start Swift/Xcode/
> signing until Gates 2–4 pass. See `PROJECT_REVIEW_AND_CONTINUATION_PLAN.md`
> §"Direction update" and `GATE_2_MAC_REFERENCE_HOST_PLAN.md`.

> Original framing (kept for Gate 5): **A — iOS-first** — build the *thinnest*
> end-to-end live loop on real iPad hardware, not a broad SwiftUI app. Depends on
> the MIDI-input slice (PR #26).
>
> **Goal loop:** MIDI keyboard → CoreMIDI in → parser/router → chord recognition
> → `EngineSession`/`StylePlayer` → CoreMIDI out → external tone generator.
>
> Status of this doc: **for PTH review before large implementation.** Consistency
> checked against BRD (iPad-first, MIDI-out MVP, live-first UX), the Master
> Delivery Plan doctrines (Tower of Time, audio-thread-sacred, runtime-knows-no-
> vendor), and the current codebase (vendor-agnostic C++ engine + CoreMIDI I/O).

---

## 0. Scope & non-goals

**In:** one playable vertical loop on iPad + the minimum UI to operate it; chord
**latch (~80 ms)** + **sustain/hold (CC64)** as mandatory parts of the slice;
robust note lifecycle (no stuck notes) across disconnect/reconnect and
background/foreground.

**Out (explicitly deferred):** multi-screen SwiftUI product (library/setlist/
mixer/browser), internal sound engine, SFF2, marketplace, AUv3, macOS product
polish. The macOS JUCE app remains a **diagnostic/dev host**, not the product.

**Minimum UI (single Live screen):** MIDI input picker + status; MIDI output
picker + status; current chord (large); current style name; Start/Stop; tempo;
current variation (A–D); connection/error banner. Nothing else in slice 1.

---

## 1. Target architecture — portable core + Apple platform layer

**Constraint (PTH):** the portable C++ core must not depend *directly* on Apple
frameworks; CoreMIDI/CoreAudio live in a **platform layer**. (Note: Apple
frameworks exist on iOS too, so this is an *architecture cleanliness / future-
portability* requirement, not a functional blocker — sequenced to avoid
destabilising the working engine.)

Proposed CMake/target split (evolves the current single `arranger-engine`):

```
arranger-core     (STATIC, PORTABLE — NO Apple framework)
  src/engine/{uasf,arranger,music,chord,control,performance,integration}
  src/engine/midi/{scheduler,panic,note_balance}      (pure)
  src/midi/{midi_input_parser,midi_input_router,midi_output_*} (pure/interfaces)
  src/{session,control,ui,performance/{groove,telemetry}}
  → depends on nothing platform-specific; defines interfaces:
       IMidiOutputProvider (exists), IMidiInputSource (new, small),
       IRealtimeClock (exists in performance/clock)

platform-apple    (STATIC, Apple-only — CoreMIDI/CoreAudio/mach)
  src/platform/apple/coremidi_out.{h,cpp}   (moved from engine/midi)
  src/platform/apple/coremidi_in.{h,cpp}    (moved from engine/midi)
  src/platform/apple/coreaudio_driver.*     (moved from engine/realtime)
  src/platform/apple/mach_clock.*           (mach_time behind IRealtimeClock)
  → implements the core interfaces; links CoreMIDI/CoreAudio/AudioToolbox/CF

importer-sff1     (unchanged) → arranger-core
apps/macos (JUCE) → arranger-core + platform-apple   (dev host)
LiveArranger (iOS, SwiftUI) → bridge → arranger-core + platform-apple
```

**Migration is incremental & test-gated** (see Phase 1). We do **not** refactor
the two clock/two groove subsystems here (PTH: only if blocking) — we reuse the
existing `IRealtimeClock`/`CoreAudioClock` as the clock seam and leave the
`engine/realtime/clock` mach usage where it is until it actually blocks iOS
(it compiles on iOS as-is).

---

## 2. Xcode project & targets

Created under `apps/ios/` (kept out of the CMake core; the core is consumed as
source or a prebuilt static lib):

| Target | Type | Contents |
|---|---|---|
| `LiveArranger` | iOS app (SwiftUI) | App entry, Live screen, view models (Swift), the bridge |
| `LiveArrangerTests` | iOS unit (XCTest) | Swift-side bridge/marshalling tests (simulator) |
| `LiveArrangerUITests` | iOS UI | Smoke of the Live screen (simulator) |
| `AiArrangerCore` | static lib (or SwiftPM binary) | `arranger-core` + `platform-apple` sources, C++20, `-fno-exceptions` on RT TUs |

Two viable integration mechanisms (decide at Phase 2):
- **(Recommended) Xcode project references the C++ sources directly** in a
  `AiArrangerCore` static-lib target with a module map — simplest for Swift/C++
  interop, one repo, no packaging step.
- Alternative: wrap `arranger-core`+`platform-apple` as a **SwiftPM** package
  (`Package.swift`) with a C++ target + a thin Swift target. Cleaner boundaries,
  more setup. Deferred unless the Xcode-direct route gets messy.

Build settings: `C++ and Objective-C Interoperability = C++/Objective-C++`
(Xcode 15+/Swift 5.9+), `CLANG_CXX_LANGUAGE_STANDARD = c++20`, iOS deployment
target **16.0** (per PRD).

---

## 3. Swift ↔ C++ bridge — ownership & threading

Keep the interop surface **thin** (Master Plan DOCTRINE 03): commands go *in*,
snapshots come *out*; Swift never calls into the audio/read threads.

- A single C++ facade `LiveEngineFacade` (in `arranger-core`, no Apple deps beyond
  what platform-apple injects) owns:
  - `EngineSession` (clock, scheduler, panic, `StylePlayer`, `PerformerAdapter`).
  - the platform MIDI in/out adapters (injected — core sees interfaces).
  - the active `IChordScanMode` (Fingered by default) + chord **latch** state.
- **Threading model / ownership:**
  - **UI (main) thread (Swift):** calls facade command methods (`start/stop`,
    `setTempo`, `setVariation`, `selectMidiInput/Output`) — each just posts a
    lock-free `ControlEvent` or sets an atomic. Never blocks.
  - **CoreMIDI input thread (read callback):** `CoreMidiIn.readProc` → parse →
    `routeMidiInput` → `PerformerAdapter::postEvent` (lock-free SPSC). **No
    malloc/lock/ObjC** in this callback (already honoured).
  - **Engine tick:** driven by a periodic timer/clock (slice 1 may reuse a host
    timer like the macOS `EngineDriver`; audio-callback clock is a follow-up).
    `tick()` drains control+note events, advances the sequencer, emits output
    events → CoreMIDI out dispatch thread.
  - **Output dispatch thread:** existing `CoreMidiOut` dedicated thread does
    `MIDISend` (not the audio thread).
  - Swift reads an **atomic snapshot** (chord, style, playing, tempo, variation,
    connection/error) at ~30–60 Hz for the UI. No direct engine reads.
- Bridge exposes only POD/`enum`/`Int`/`String` across the boundary — no C++
  containers leak into Swift.

---

## 4. CoreMIDI lifecycle (iOS)

- Create one `MIDIClient` per app; input port + output port.
- **Input:** enumerate sources, connect selected; `readProc` → parser/router.
- **Output:** existing `CoreMidiOut` (client/port/dispatch thread/hot-plug).
- **Hot-plug:** `MIDINotifyProc` marks setup dirty; re-resolve by name (output
  already does; extend input). **Disconnect must NOT stop transport** and must
  **flush to zero stuck notes** (panic on loss) — DOCTRINE 06.
- **App lifecycle:** on background → send All-Notes-Off/panic + stop dispatch
  cleanly (or keep alive if configured as background-audio); on foreground →
  re-resolve endpoints, re-send init. **No stuck notes across transitions.**
- iOS specifics: enable **Background Modes → Audio** only if we keep MIDI running
  in background; otherwise panic-on-background. Set up `AVAudioSession`
  (category `playback`) even for MIDI-only, to keep the process alive on stage.

---

## 5. Mandatory slice features (engine-side, headless-testable)

These land in `arranger-core` with headless tests, then are surfaced by the UI:

1. **Chord latch (~80 ms):** on a chord *release/downgrade*, defer the change for
   a latch window; a new valid chord within the window replaces without a
   NoChord flicker. Deterministic via the engine clock (advance to assert). Lives
   in `PerformerAdapter` alongside the existing sustain logic.
2. **Sustain/hold (CC64):** already implemented (this session). Extend tests for
   interaction with latch.
3. **Panic / all-notes-off:** existing `PanicHandler`; wire CC123 (done) + app
   background + device loss.
4. **Note lifecycle invariants:** NoteOn==NoteOff after any disconnect/reconnect,
   background/foreground, panic — asserted headless with a fake in/out provider.

---

## 6. Testing strategy

| Layer | Where | Runs on |
|---|---|---|
| Pure logic (parser, router, latch, sustain, panic, note balance) | `tests/**` (CTest, headless) | CI / macOS |
| Bridge marshalling (enum/POD/snapshot round-trip) | `LiveArrangerTests` | **Simulator** |
| Live-screen smoke (controls exist, state binds) | `LiveArrangerUITests` | **Simulator** |
| Fake MIDI in/out end-to-end (disconnect/reconnect, no stuck notes) | `tests/**` + a fake iOS provider | CI / Simulator |
| **Real keyboard → chord → out; latency; 30-min stability; stuck-note under abuse** | manual on device | **iPad hardware (PTH)** |

Simulator cannot exercise real CoreMIDI hardware endpoints or measure true
latency — those are the device-only, PTH-run items.

---

## 7. Signing / device prerequisites (PTH / hardware)

- Apple Developer account + a **development signing certificate** & provisioning
  profile (or automatic signing with a Team ID).
- A registered **iPad** (target iPad Air M1+; min iPad mini 6) in the profile.
- A **class-compliant USB/BLE MIDI** interface + a MIDI keyboard + a tone
  generator/sound module for the output end.
- These gate: on-device build/run, real-CoreMIDI validation, latency capture,
  stability runs. **Blocking for the device milestone; not for the code below.**

---

## 8. Phasing — what I implement now (no signing/hardware) vs. blocked

| Phase | Work | Needs device/signing? | Status |
|---|---|---|---|
| **1. Platform boundary** | Split CMake into portable core + `platform-apple` (CoreMIDI/CoreAudio); add `IMidiInputSource`; keep all tests green. | **No** | ✅ **DONE** (commit `ed2647d`) — core links no Apple framework; `sff1-parser` links core alone; 73/73 green |
| **2. LiveEngineFacade + snapshot** | Portable C++ facade over `EngineSession` + injected in/out; atomic UI snapshot; headless tests. | **No** | ✅ **DONE** (commit `003d650`) — 74/74 green |
| **3. Chord latch (~80 ms)** | Deterministic latch in `PerformerAdapter` (sample-position based via clock); interacts with sustain. | **No** (but see note) | ⏸ **Deferred into the device slice** per PTH ("not a separate engine-first phase"). Design: latch window in samples from `clock.getSamplePosition()`/`getSampleRate()`; default hold-mode ON preserves current behaviour; hold-mode OFF + latch clears to NoChord after the window. Needs a `StylePlayer::setChord(NoChord)` semantics check first. |
| **4. Fake-provider E2E** | Fake in/out; assert no stuck notes across stop/panic. | **No** | ✅ **Covered** by `test_live_engine_facade` (no stuck notes on stop; panic dispatches CC120/123). Disconnect/reconnect/background scenarios remain for the device slice. |
| **5. Xcode project + bridge scaffolding** | `apps/ios/` project, Swift/C++ interop, Live screen SwiftUI. | Needs **full Xcode** | ⛔ **BLOCKED** — this environment has only Command Line Tools (`xcodebuild` unavailable, no iOS SDK/simulator). Cannot author + verify the Xcode project / Swift here. |
| **6. On-device bring-up** | Signing, provisioning, run on iPad, real CoreMIDI, latency/stability. | **Yes** | ⛔ **BLOCKED** — signing + hardware (PTH). |

**Environment note (2026-07-11):** only Apple Command Line Tools are installed
(Swift 6.2 compiler present, but no `xcodebuild`, no iOS SDK, no simulators).
Phases 1–2–4 (pure CMake/headless) are complete + verified. Phases 5–6 require
full Xcode + signing + an iPad and are the STOP point.

---

## 9. Acceptance criteria (slice 1)

- Portable `arranger-core` links with **no Apple framework**; `platform-apple`
  isolates CoreMIDI/CoreAudio; **all existing 73 tests still pass, 0 warnings**.
- Chord latch + sustain covered by headless tests; **0 stuck notes** across
  disconnect/reconnect/background/panic in the fake-provider E2E.
- iOS app builds and runs on **Simulator**, shows the Live screen bound to a live
  snapshot (controls functional against a fake/loopback source).
- **(Device, PTH):** real keyboard drives accompaniment out to a tone generator;
  chord→out latency within the P95 ≤ 20 ms budget target; 30-min run with 0
  crashes / 0 stuck notes / 0 dropped transport on hot-plug.

---

## 10. Files to create / modify (indicative)

- **Move:** `src/engine/midi/coremidi_{in,out}.{h,cpp}`,
  `src/engine/realtime/coreaudio_driver.*` → `src/platform/apple/…`; update
  includes + CMake targets.
- **New (core):** `src/midi/midi_input_source.h` (`IMidiInputSource`);
  `src/engine/integration/…` latch additions; `src/session/live_engine_facade.{h,cpp}`;
  `src/session/engine_snapshot.h`.
- **New (iOS):** `apps/ios/LiveArranger.xcodeproj`, `App.swift`,
  `LiveView.swift`, `LiveViewModel.swift`, `EngineBridge.swift` +
  `EngineBridge.h/.mm` (interop), module map, `Info.plist`, entitlements.
- **New tests:** `tests/**/test_chord_latch.cpp`,
  `tests/**/test_midi_io_stuck_notes.cpp`, `LiveArrangerTests/*`.
- **Docs:** update tech-arch/HANDOFF to reflect the platform-core split and the
  "iOS-first, macOS = dev host" reality; keep **no SFF2/hardware compatibility
  claims**.

---

## 11. Risks

**P0**
- Swift/C++ interop friction (C++20 features, exceptions, name mangling) → keep
  the bridge to POD/enums; wrap in Obj-C++ (`.mm`) if pure interop struggles.
- Stuck notes on device loss / backgrounding → panic-on-loss + note-balance
  assertions are mandatory before any device claim.
- RT-safety regressions when moving files → re-run full suite after each move.

**P1**
- Xcode/toolchain availability in this environment (Phase 5 may be simulator-
  build-only or blocked if no Xcode) → Phases 1–4 are pure CMake and unaffected.
- Latency target unverifiable without hardware → explicitly a device milestone.
- Platform split churn vs. the two-clock debt → we reuse `IRealtimeClock` and do
  **not** consolidate clocks/groove now (avoid scope creep).

---

## 12. BRD / architecture consistency check

- **iPad-first, MIDI-out MVP, live-first single screen** — matches BRD §1–2, PRD
  §6, roadmap M1–M2, and DOCTRINE 01/10 (timing first, MIDI-only is shippable).
- **Vendor-agnostic runtime, Yamaha only in importer** — preserved; the platform
  split touches only Apple I/O, not vendor logic.
- **No sound engine, no SFF2, no marketplace** in this slice — matches DOCTRINE
  02/10 deferral order.
- **No hardware/compatibility claims** — `hardware_validated:false` retained.
