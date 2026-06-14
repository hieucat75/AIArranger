# Gate 15 — Manual Feel Review (for PTH)

> **Subjective scorecard — runs locally.** Gate 15 targets *feel*, which can't be
> fully asserted in code. Build the app, play it, and score below. This is NOT
> hardware validation — `hardware_validated: false`; Korg PA stays DEFERRED_BY_PTH.

## Build + run
```
cmake -S . -B build -DBUILD_MACOS_APP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target aiarranger-macos --parallel
open "build/apps/macos/aiarranger-macos_artefacts/Release/AI Arranger.app"
```
Optionally route to a DAW/IAC (see `GATE_14C_MANUAL_DAW_VERIFICATION.md`).

## Scorecard (0–5 each; fill in)

| # | Dimension | Score | Notes |
|---|---|:--:|---|
| 1 | Overall musicality / "feels like an instrument" | ☐ | |
| 2 | Transition smoothness (fill/variation at bar boundary) | ☐ | |
| 3 | Groove feel — tight | ☐ | |
| 4 | Groove feel — laid-back | ☐ | |
| 5 | Groove feel — swing-light | ☐ | |
| 6 | Groove feel — live-pop | ☐ | |
| 7 | Groove feel — acoustic-soft | ☐ | |
| 8 | Responsiveness / low-latency feel | ☐ | |
| 9 | Visual clarity (indicators readable at a glance) | ☐ | |
| 10 | Stage mode usability (large transport, fullscreen) | ☐ | |
| 11 | Theme readability on stage (dark-luxury / stage-readable) | ☐ | |
| 12 | Live confidence (would you gig with it?) | ☐ | |

## Quick checks
- ☐ CoreAudio/host-time clock selected (lower jitter than the old 1ms timer).
- ☐ All 7 performer indicators update live.
- ☐ Latency overlay shows rolling avg/max/jitter histogram.
- ☐ Session persistence: change state → quit → relaunch → state restored.
- ☐ No UI freeze during 5 min of play + control spam.

## Recording (optional)
- Screen recording / screenshots → `docs/gate-plans/gate-15-artifacts/`.

## Status
- Engine-side (clock/groove/transition/latency/abuse/persistence/theme/overlay):
  **PASS** (headless — see `GATE_15_HANDOFF.md`, 71 binaries / 829 assertions).
- Subjective feel: **PENDING (PTH fills in)**.
- **deterministic: true** · **hardware_validated: false** · no KORG/Yamaha claim.
