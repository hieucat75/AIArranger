# Gate 15 — Plan: Performer Experience & Instrument Feel

> **PROPOSAL ONLY — no code in this document.** The largest gate so far: turn the
> functional, audible arranger app into a *responsive software instrument*
> (feel, smoothness, low-latency interaction, visual clarity) — without changing
> the engine's determinism, realtime safety, or isolation.
>
> Authoring note: composed from the provided Gate 15 scope + the codebase. Every
> report emits `deterministic: true` + `hardware_validated: false`. No KORG/Yamaha
> compatibility claim. See `docs/strategy/KORG_FIRST_VALIDATION_STRATEGY.md` and
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`.

---

## 1. Context

Gates 1–14C are merged: the engine plays UASF styles deterministically; the
performer layer (sync/fill/variation/chord/split/panic) is wired into StylePlayer
(Gate 13); a macOS JUCE shell exists (Gate 14) and now routes MIDI to a
selectable CoreMIDI output (Gate 14C). The project has crossed from *prototype*
to *playable instrument*. **The next problem is performer FEEL** — how musical,
smooth, and responsive it is to play live.

## 2. Strategic Goal

Transform a **"functional arranger app" → "responsive software instrument"**.
KPI is subjective: playability, live confidence, smoothness, responsiveness,
groove feel. **Preserve** determinism, realtime safety, and engine isolation.

## 3. Core Direction

Prioritize performer experience, interaction quality, low-latency feel, visual
clarity, and musical transitions. This gate shifts **away** from backend infra,
parsing, and format reverse-engineering.

## 4. Scope (9 items)

1. **CoreAudio / Realtime Clock Upgrade** — replace the ~1ms prototype timer with
   a CoreAudio / host-time-driven clock (lower jitter).
2. **Transition Feel Layer** — smoother bar-boundary section/fill/variation
   transitions (pre-roll arming, visualization hook).
3. **Deterministic Groove Humanization** — seeded, reproducible micro-timing/
   velocity profiles (tight / laid-back / swing-light / live-pop / acoustic-soft).
4. **Performer-Oriented UI Redesign** — larger transport, clear hierarchy,
   stage/fullscreen mode, optional diagnostics drawer.
5. **Realtime Visual Feedback** — live indicators (fill queued, variation
   pending, sync armed, MIDI active, latency, reconnect, groove profile).
6. **Low-Latency Interaction Pass** — end-to-end latency measurement + overlay.
7. **Stability Under Abuse** — stress suite (rapid spam, panic, hot-plug, floods).
8. **Performer Session Persistence** — save/restore style, variation, groove,
   split, MIDI routing, UI layout to disk.
9. **Visual Theme System** — themeable look (dark luxury / stage-readable /
   high-contrast), iPad-scaling hook.

## 5. Architecture Constraints (HARD RULES)

- Engine stays **isolated**; **UI owns 0 timing**.
- **No locks on the realtime path** (atomics / lock-free only).
- **Deterministic replay** preserved (groove is seeded; identical seed+profile →
  identical output).
- **CI-safe**: all automated tests headless, synthetic, no real device/network.

## 6. Forbidden

- ❌ No DSP / internal audio engine. ❌ No AU/VST hosting. ❌ No cloud/network.
- ❌ No AI arranger generation. ❌ No timeline editor / piano roll. ❌ No audio
  recording / waveform. ❌ No claims-policy change. ❌ No KORG/Yamaha or
  hardware-parity claim.

## 7. Suggested Structure

`src/performance/{clock,groove,telemetry}`, `src/ui/{performance,theme}`,
`apps/macos/Source/` (overlay/stage/theme JUCE views), `tests/performance/`,
plus handoff + manual feel-review docs. Detailed layout in §12.

## 8. Testing Requirements

- Deterministic groove replay (same seed → identical events).
- Transition smoothness (visualization state correct at boundaries).
- Rapid abuse (spam/panic) — no crash/deadlock/stuck.
- Hot-plug stress (output add/remove during playback).
- UI responsiveness under load (ViewModel update rate, no blocking).
- Clock jitter (synthetic clock determinism + jitter stats).
- Latency regression (E2E control→dispatch within budget).
- **All CI-safe, deterministic, synthetic.**

## 9. Deliverables

- Smoother live interaction; redesigned performer UI; stage mode; theme system.
- Latency report + jitter report (`deterministic: true`, `hardware_validated:
  false`).
- Screenshots / videos (PTH, local).
- Updated `GATE_15_HANDOFF.md` + `HANDOFF_MASTER.md` + manual feel review doc.

## 10. Success Criteria

- The app feels more musical; transitions smoother; UI feels like an instrument.
- Latency tighter (measured); no regression in the 61 baseline binaries.
- Determinism preserved; stress tests stable; CI green.
- Subjective feel scorecard captured by PTH.

---

## 11. Module design

### 11.1 CoreAudio / Realtime Clock
`IRealtimeClock` abstraction. `CoreAudioClock` drives ticks from a CoreAudio
output-unit render callback / host time (lower jitter than the 1ms timer).
`SyntheticClock` (deterministic, advance-by-N) for CI. The EngineDriver depends
on the interface, not a concrete clock. CoreAudio impl is local/GUI-side; tests
use synthetic.

### 11.2 Transition Feel Layer
Reuses the existing bar-boundary sequencer quantize; adds an optional pre-roll
"arming" window so a queued fill/variation visibly arms before it fires, plus a
`transition_state` telemetry struct (queued? bars-to-fire?) the UI visualizes.
No new timing authority — still the sequencer.

### 11.3 Deterministic Groove Humanization
`IGrooveProfile` + a `DeterministicRng` (seeded, splitmix/xorshift — no
`std::random_device`). `GrooveEngine` applies per-event micro-timing/velocity at
the dispatch seam (like swing). Profiles: tight / laid-back / swing-light /
live-pop / acoustic-soft. Contract: **same seed + profile → identical event
stream** (replay-tested). RT-safe (stack-only, no alloc/lock).

### 11.4 Performer-Oriented UI Redesign
Two layouts behind ViewModels: `DesktopMode` (panels + diagnostics drawer) and
`StageMode` (large transport, minimal chrome, fullscreen). Layout choice is UI
state; no engine impact. Larger hit targets, clear visual hierarchy.

### 11.5 Realtime Visual Feedback
`PerformerOverlay` VM aggregates 7 indicators (fill queued, variation pending,
sync armed, MIDI active, latency, reconnect, groove profile) from existing
ViewModels/telemetry. Pure data → JUCE indicator components.

### 11.6 Low-Latency Interaction Pass
Extend the Gate 12/13 latency reporter to a live `LatencyOverlay` (rolling avg,
max spike, jitter histogram). E2E control→dispatch measurement. Emits
`deterministic:true` + `hardware_validated:false`.

### 11.7 Stability Under Abuse
`tests/performance/test_abuse_stress_suite.cpp`: rapid fill/variation spam,
repeated panic, output hot-plug churn, chord floods, sync-arm abuse — assert no
crash/deadlock/stuck-notes and bounded state. Headless.

### 11.8 Performer Session Persistence
Extend the Gate 14 `SnapshotManager` to persist to a JSON file on disk: style,
variation, groove profile, split, MIDI routing (device name), UI layout/theme.
Validate on load (version + ranges); corrupt file → safe default.

### 11.9 Visual Theme System
`ThemeManager` abstraction (UI-agnostic palette/typography tokens) + a JUCE
`ThemeLookAndFeel`. Default "dark luxury, stage-readable". iPad-scaling hook
(token-based sizing). Theme is UI-only.

## 12. File layout

```
src/performance/clock/
├── realtime_clock.{h,cpp}       # IRealtimeClock
├── coreaudio_clock.{h,cpp}      # CoreAudio impl (local)
└── synthetic_clock.{h,cpp}      # deterministic test clock
src/performance/groove/
├── groove_profile.{h,cpp}       # IGrooveProfile + 5 profiles
├── groove_engine.{h,cpp}        # apply at dispatch
└── deterministic_rng.{h,cpp}    # seeded PRNG
src/performance/telemetry/
├── latency_overlay.{h,cpp}      # rolling stats + jitter histogram
└── transition_state.{h,cpp}     # queued-state viz data
src/ui/performance/
├── performer_overlay.{h,cpp}    # indicators VM
├── stage_mode.{h,cpp}           # fullscreen layout VM
└── transition_feel.{h,cpp}      # transition viz VM
src/ui/theme/
└── theme_manager.{h,cpp}        # theme tokens (UI-agnostic)
apps/macos/Source/
├── PerformerOverlayPanel.{h,cpp}
├── LatencyOverlay.{h,cpp}
├── StageMainComponent.{h,cpp}
└── ThemeLookAndFeel.{h,cpp}
tests/performance/
├── test_realtime_clock_synthetic.cpp
├── test_groove_profile_deterministic.cpp
├── test_transition_feel.cpp
├── test_abuse_stress_suite.cpp
├── test_session_persistence.cpp
└── test_latency_overlay.cpp
docs/gate-plans/
├── GATE_15_HANDOFF.md
└── GATE_15_MANUAL_FEEL_REVIEW.md   # PTH subjective scorecard
```

> `src/performance/*` and `src/ui/*` are headless + CI-tested. `apps/macos/*` is
> JUCE GUI, built locally (`BUILD_MACOS_APP=ON`, CI=OFF).

## 13. Task breakdown (G15-TASK-A → K)

| Task | Commit | Content |
|---|---|---|
| A | `G15-TASK-A` | `IRealtimeClock` + `SyntheticClock` + clock jitter benchmark (synthetic). |
| B | `G15-TASK-B` | `CoreAudioClock` (macOS, local) + jitter comparison report; EngineDriver depends on the interface. |
| C | `G15-TASK-C` | groove profile system — `DeterministicRng`, `IGrooveProfile`, 5 profiles, `GrooveEngine`; deterministic replay tests. |
| D | `G15-TASK-D` | transition feel layer — pre-roll arming + `transition_state` telemetry + viz VM. |
| E | `G15-TASK-E` | `LatencyOverlay` — rolling avg/max/jitter histogram; E2E measurement. |
| F | `G15-TASK-F` | abuse stress suite (spam/panic/hot-plug/chord flood/sync abuse). |
| G | `G15-TASK-G` | session persistence — extend SnapshotManager to JSON file (validate/default). |
| H | `G15-TASK-H` | `ThemeManager` (UI-agnostic tokens) + `ThemeLookAndFeel` (JUCE) — dark-luxury default. |
| I | `G15-TASK-I` | performer UI redesign — larger transport, hierarchy, `StageMode` + diagnostics drawer (JUCE). |
| J | `G15-TASK-J` | realtime visual feedback — `PerformerOverlay` VM + 7 indicators (JUCE panel). |
| K | `G15-TASK-K` | `GATE_15_HANDOFF.md` + `HANDOFF_MASTER.md` + `GATE_15_MANUAL_FEEL_REVIEW.md`. |

One commit per task; the 61 baseline binaries stay green after each.

## 14. CI strategy

- Clock (synthetic), groove, transition, abuse, persistence, latency tests run
  headless via CTest. Visual/theme tested at the **ViewModel level only** (no GUI
  rendering).
- CoreAudio clock + JUCE GUI build only with `BUILD_MACOS_APP=ON` (CI=OFF).
- Manual feel review = PTH local.

## 15. Acceptance criteria (checklist)

- ☐ 1ms timer replaced by a CoreAudio/host-time clock (jitter measurably lower).
- ☐ 5 groove profiles deterministic (same seed → identical replay).
- ☐ Transition smoothness viz state available.
- ☐ Stress suite passes (no crash/deadlock/stuck).
- ☐ Session persistence round-trip (disk).
- ☐ ThemeManager works (dark-luxury default).
- ☐ Performer overlay shows all 7 indicators.
- ☐ Latency overlay rolling stats correct.
- ☐ Stage mode toggleable.
- ☐ 61 baseline binaries green.
- ☐ CI green.
- ☐ Subjective scorecard PENDING PTH (manual feel review).

## 16. Effort estimate

- **New source:** ~35–40 files (clock + groove + telemetry + theme + UI panels +
  tests + docs).
- **New tests:** ~10–12 headless binaries.
- **Complexity:** HIGH (largest gate) — CoreAudio clock, JUCE LookAndFeel, UI
  redesign, deterministic groove. No new dependencies (JUCE 8.0.4 already in).

## 17. Status rules (baked in)

- Every report hard-codes **`deterministic: true`** + **`hardware_validated:
  false`**.
- Claims Policy unchanged; engine isolation strict; UI owns 0 timing; no
  KORG/Yamaha or hardware-parity claim.

## 18. Risks

- **CoreAudio clock needs an audio session** → likely not viable headless on CI;
  mitigate with `SyntheticClock` for all CI tests, CoreAudio local-only.
- **JUCE LookAndFeel** learning curve.
- **"Feel" is subjective** → cannot be fully asserted programmatically; captured
  via `GATE_15_MANUAL_FEEL_REVIEW.md`.
- **Theme aesthetics** need taste tuning; initial implementation, refine later.

## 19. Dependency

Branches off `main` (after Gate 14C / `v0.14.2-coremidi-out`). No other
dependency. JUCE 8.0.4 already pinned.

---

> When approved, implement per §13 — one commit per task, headless tests green
> after each, PR at the end. No hardware/KORG compatibility claims; reports stay
> `deterministic:true` + `hardware_validated:false`.
