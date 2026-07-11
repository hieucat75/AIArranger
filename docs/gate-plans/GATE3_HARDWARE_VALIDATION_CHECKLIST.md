# Gate 3 — Hardware Validation Checklist (macOS live reference host)

> **Status:** BLOCKED on physical hardware. `hardware_validated:false`.
> The Mac host is **NOT** marked live-safe until every item below PASSes on real gear.
> Lifecycle hardening (CoreMIDI teardown race, close-time flush, output-switch flush,
> snapshot claim) landed pre-hardware on `feat/gate3-lifecycle-hardening`; this checklist
> is the acceptance gate that turns `hardware_validated` to `true`.

## Equipment required
- **Mac** running the built `AI Arranger.app` (`cmake -B build -DBUILD_MACOS_APP=ON && cmake --build build --target aiarranger-macos`).
- **MIDI keyboard/controller** with 5-pin DIN or USB-MIDI (real NoteOn/NoteOff + sustain pedal on CC64).
- **External sound module / arranger** (or a soft-synth on a second host) as the MIDI **output** destination.
- **USB-MIDI interface(s)** and/or a powered USB hub for hot-plug tests.
- **Sustain pedal** (for CC64).
- **Stopwatch / audio interface + DAW** or a MIDI monitor with timestamps (e.g. MIDI Monitor.app) to measure chord→output latency.
- Optional: a **MIDI monitor** on the output to confirm exact bytes (CC120/123, note balance).

## How to record results
For each item: mark PASS/FAIL, note the build SHA, the devices used, and any observation.
A single stuck note, crash, deadlock, dropped callback, or audible timing drift = FAIL for that item.

---

## A. Core play path
1. **Keyboard → chord → arranger → sound module.**
   - Do: pick keyboard as MIDI In, module as MIDI Out, Load a Genos `.sty`, press Start, play chords in the lower zone.
   - Pass: accompaniment plays on the module; the detected chord shown in the UI matches what is played; changing chord re-voices within one bar; no stuck notes.
2. **Chord-to-output latency.**
   - Do: play a chord; measure time from key-down to first accompaniment note at the output (MIDI monitor timestamps or audio capture). Repeat ≥20 times.
     Optionally build with `-DAIARR_LATENCY_TRACE=ON` and capture the software
     trace in parallel (see
     [GATE_3_LATENCY_INSTRUMENTATION.md](GATE_3_LATENCY_INSTRUMENTATION.md)); run
     `tools/latency_report/latency_report.py` on the export for p50/p95/max +
     stuck-note counters. **The software report is a cross-check, not the pass
     itself** — the audible measurement here is authoritative.
   - Pass: median latency within the target budget (record the number); no pathological outliers (> 30 ms) beyond occasional scheduler granularity.
3. **Multiple tempos.**
   - Do: run at ~60, ~120, ~200+ BPM; change tempo mid-playback via the slider.
   - Pass: groove stays in time at every tempo; tempo change takes effect smoothly without stuck notes or a scheduling stall.
4. **Sustain pedal (CC64).**
   - Do: hold a chord, press+hold sustain, release keys, then lift the pedal.
   - Pass: chord is held while the pedal is down and released cleanly when it lifts; deferred note handling leaves no stuck note.
5. **Section transitions.**
   - Do: exercise Intro, Variation A–D, Fill, Break, Ending; fire commands mid-bar.
   - Pass: every change lands on a bar boundary; no stuck notes across the switch; Ending stops cleanly.
6. **Panic.**
   - Do: while playing dense material, hit Panic.
   - Pass: all sound stops immediately (CC120/all-notes-off reaches the module); the engine is immediately re-armable (Start plays again).

## B. Lifecycle / teardown (the four hardened paths — verify on hardware)
7. **App close while holding a chord / while playing.**
   - Do: play, hold a chord, then Cmd-Q / close the window.
   - Pass: the module is silenced (all-sound-off received) **before** the app exits; no hanging note on the device. (Covers the close-time flush fix.)
8. **Switch MIDI output while playing.**
   - Do: while playing into module A, switch the Out picker to module B.
   - Pass: module A is silenced (no hanging notes); no note-off from A leaks to B; transport ends up stopped; pressing Start resumes into B cleanly. (Covers the output-switch flush fix.)
9. **Switch MIDI output while idle.**
   - Do: with transport stopped, switch outputs back and forth.
   - Pass: no errors, no stuck notes, selection reflects in the UI.
10. **Input hot-plug (disconnect/reconnect the keyboard).**
    - Do: while running (and while playing), unplug the keyboard's USB/DIN, wait, replug.
    - Pass: no crash; transport keeps running; on replug the keyboard drives chords again (re-select if needed); **no read-callback crash at the disconnect instant** (covers the CoreMIDI teardown-race fix).
11. **Output hot-plug (disconnect/reconnect the module).**
    - Do: while playing, unplug the output device, wait, replug.
    - Pass: no crash; transport keeps running (events dropped while gone); on replug output resumes (name re-resolves); no stuck note wedged from before the unplug.
12. **Change input source while playing.**
    - Do: switch the In picker between two connected controllers mid-play.
    - Pass: no crash; chords follow the newly selected controller; no callback wedged on the old source.

## C. Stress / environment
13. **Load a style while running.**
    - Do: press Start, then Load a different `.sty` repeatedly while playing.
    - Pass: no crash/garbled MIDI (covers the loadStyle quiesce fix); transport ends stopped after each load; press Start to play the new style; parse failure shows a clear UI error and keeps the old style.
14. **Sleep / wake.**
    - Do: with the app running and a device selected, sleep the Mac, wake, resume playing.
    - Pass: no crash/deadlock; CoreMIDI endpoints re-resolve; playback works after wake (re-select devices if the OS dropped them).
15. **Soak test (30–60 min).**
    - Do: play continuously (or loop an automated chord/section pattern) for 30–60 minutes into the module.
    - Pass: **no stuck notes, no crash, no deadlock, no dropped callbacks, no audible timing drift**; NoteOn/NoteOff stay balanced start to finish; rx/tx counters advance monotonically without stalling.

---

## Sign-off
- [ ] All A/B/C items PASS on real hardware, build SHA recorded.
- [ ] No stuck note / crash / deadlock / callback loss / timing drift observed.
- [ ] Only then: flip `hardware_validated:true` and mark the Mac reference host live-safe.

Until every box is checked, the Mac host remains a **pre-hardware reference host** and
`hardware_validated:false` stays in place.
