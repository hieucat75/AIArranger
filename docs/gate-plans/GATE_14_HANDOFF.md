# Gate 14 — Handoff (macOS App Shell Prototype)

> Branch `gate-14-macos-app-shell` off `main` (`ec3f00c`).
> **Status: ✅ Gate 14 Engine-side PASS (headless) / GUI builds locally (CI-optional).**
> 55 test binaries, 661 assertions, 0 failures (headless).
> UI is a pure client of the engine — intent only; engine owns all timing/
> scheduling. No DSP, no audio, no web stack, no hardware. Reports emit
> `deterministic: true` + `hardware_validated: false`. No KORG/Yamaha claim. See
> `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Scope (10 items) — delivery

| # | Item | Where | Headless test |
|---|------|-------|:--:|
| 1 | macOS App Shell | `apps/macos/Source/{Main,MainComponent}` | GUI (local) |
| 2 | Engine Session Lifecycle | `src/session/engine_session` | ✅ |
| 3 | Realtime Transport UI | `apps/macos/.../TransportPanel` + `src/ui/transport_view_model` | ✅ (VM) |
| 4 | Chord Input UI | `apps/macos/.../ChordPanel` + `src/ui/chord_view_model` | ✅ (VM + event) |
| 5 | Realtime Status Visualization | `apps/macos/.../DiagnosticsPanel` + `src/ui/diagnostics_view_model` | ✅ (VM) |
| 6 | MIDI Device Layer | `src/midi/midi_device_manager` | ✅ |
| 7 | Internal Event Console | `apps/macos/.../ConsolePanel` | GUI (local) |
| 8 | Latency Telemetry | `src/ui/latency_monitor` + DiagnosticsPanel | ✅ |
| 9 | Style Browser (Minimal) | `apps/macos/.../StyleBrowserPanel` | GUI (local) |
| 10 | Snapshot State System | `src/session/snapshot_manager` | ✅ |

## 2. Commits

| Hash | Title |
|---|---|
| `cded64f` | A: build system + JUCE dependency (CI-optional macOS app) |
| `d39340c` | B: engine session lifecycle (boot/shutdown/reset) |
| `79be275` | C: async UI↔engine bridge (lock-free SPSC, bidirectional) |
| `459b58f` | D: UI-agnostic ViewModels (transport/chord/diagnostics/latency) |
| `84b6157` | E: JUCE app shell skeleton (Main + MainComponent + EngineDriver) |
| `25ab91b` | F: TransportPanel (buttons → ControlEvents via bridge) |
| `b57639e` | G: ChordPanel + thread-safe chord intent via bridge |
| `aac84a6` | H: DiagnosticsPanel + console + latency display |
| `d0368ea` | I: MIDI device manager (provider-abstracted, hot-plug safe) |
| `f9e08ed` | J: snapshot state system (capture/serialize/restore) |

(+ this handoff commit = Task K.)

## 3. Architecture

```
[JUCE UI (message thread)]                 [Engine (HighResolutionTimer thread)]
 TransportPanel/ChordPanel ── ControlEvent ──▶ EngineBridge ──▶ PerformerAdapter
        ▲                                          (SPSC)            │
 ViewModels ◀── EngineTelemetry ◀── EngineBridge ◀── EngineDriver ──┘ ▶ StylePlayer ▶ CoreMIDI
```

- **Strict separation:** the UI sends intent (`ControlEvent`) and renders
  telemetry. It never calls engine logic, never schedules, never computes timing.
- **Async + lock-free:** all UI↔engine hand-off is via `EngineBridge` (two SPSC
  rings). The UI thread never blocks; the engine runs on its own driver thread.
- **No engine-core behaviour change** beyond one additive `ControlAction::Chord`
  (appended; adapter handles it in an additive case).
- **`src/{session,control,ui,midi}` are headless + CI-tested**; `apps/macos/` is
  the JUCE GUI, built locally with `BUILD_MACOS_APP=ON` (CI builds OFF).

## 4. Test evidence

| Test (new, headless) | Assertions |
|---|--:|
| test_engine_session_lifecycle | 11 |
| test_engine_bridge_async | 7 |
| test_transport_view_model | 10 |
| test_chord_view_model | 8 |
| test_diagnostics_view_model | 9 |
| test_latency_monitor | 9 |
| test_chord_control_event | 4 |
| test_midi_device_manager_synthetic | 9 |
| test_snapshot_manager | 18 |
| **Gate 14 new total** | **85** |

- Full suite: **55 binaries, 661 assertions, 0 failures** (was 46 / 576).
- Covers (per plan): boot/shutdown (repeatable, no leaks), panic during playback,
  fill spam (engine), MIDI reconnect (synthetic), snapshot restore, sync-start
  race (via earlier gates), UI event flood (100k), deterministic replay.
- All headless / synthetic. No CoreMIDI/network/GUI in CI. Baseline 46 binaries
  unchanged.

## 5. CI strategy

`option(BUILD_MACOS_APP OFF)` — CI (`scripts/run_tests.sh`, default OFF) builds
the engine + all headless tests with **no JUCE fetch** (verified: configure +
build + 55 binaries green). The JUCE GUI target (`aiarranger-macos`, JUCE 7.0.12
via FetchContent) builds locally with `-DBUILD_MACOS_APP=ON`. No workflow change.

## 6. Decisions worth noting

1. **UI is a pure client.** ViewModels are UI-toolkit-agnostic (no JUCE), fully
   unit-tested; JUCE panels are thin views over them.
2. **Engine on its own thread, lock-free bridge.** EngineDriver
   (HighResolutionTimer) runs the engine; the UI 30Hz Timer only polls telemetry.
   No mutex, no sync render callbacks.
3. **Chord intent via `ControlAction::Chord`** (additive) so the UI never touches
   the engine across threads — decoded on the engine thread by the adapter.
4. **MIDI device layer is provider-abstracted** → CoreMIDI stays out of CI; the
   manager is fully tested with a synthetic provider (hot-plug add/remove).
5. **Snapshot validates before applying** (version + ranges); invalid snapshots
   change nothing.
6. **GUI not built/verified in CI** (no JUCE fetch on the runner) — it is a
   local/dev build; screenshots are captured manually by PTH.

## 7. Screenshots

Not committed — the JUCE GUI is a local build (CI has no JUCE/display). To
capture: `cmake -S . -B build-app -DBUILD_MACOS_APP=ON && cmake --build build-app`
then run `aiarranger-macos` and screenshot the 4 panels. (Pending PTH local run.)

## 8. Residual risks / status rules

- **Software/headless PASS allowed.** Hardware parity + KORG compatibility claims
  **FORBIDDEN** until committed real-device evidence (Claims Policy). Reports stay
  `hardware_validated:false`.
- **GUI unverified in CI** — first local build may need JUCE-version/CMake tweaks
  (pinned 7.0.12). Fallback already in place: `BUILD_MACOS_APP=OFF` keeps CI green.
- **macOS code signing / notarization** deferred (prototype runs locally).
- **Automated GUI screenshots** deferred to a later `xcodebuild`/UI-test lane.
- **Chord bass in dispatch** still pending (SectionSequencer drops chord.bass —
  carried from Gate 13).
- The engine driver uses a ~1ms HighResolutionTimer for the prototype; a true
  CoreAudio/MIDI callback clock is a later refinement.

## 9. Merge status

**MERGE_ALLOWED: NO** — pending PTH review (Gate discipline). Branch pushed; PR to
be opened. CI expected green (headless; GUI not built).
