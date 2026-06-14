# Gate 10B — Hardware Validation Checklist (for PTH)

> **Manual gate — requires real hardware.** Yamaha (Genos/PSR) or Korg PA, or any
> external MIDI arranger/sound module. Connect via USB-MIDI **before** running.
> Tool: `korg-playback` (built in Gate 9). This checklist validates the Gate 10A
> engine work (per-track split, full NTT, swing, intro-start, articulation) on a
> real device. Record scores at the end → copy into
> `.openclaw/review/GATE_9_MUSICAL_REVIEW.md` §6 (covers G9 P1 #1/#2 too).

```
Usage:  korg-playback [style.uasf]      # no arg = built-in demo style
Live keys: space=start/stop  1..6=section  i=intro  f=fill  e=ending
           c=cycle chord  p=PANIC  [ / ]=prev/next dest  l=list  ?=help  q=quit
```

---

## 0. Build + connect
```
cd /Users/pth/Developer/AIArranger/build
cmake --build . --target korg-playback
```
- Connect the device via USB-MIDI and power it on **before** launching the tool.
- **Pass:** `build/korg-playback` exists; the device shows in the destination list.

## 1. Load a converted corpus style
```
./korg-playback ../corpus/yamaha/sff1/POP_ACOUSTIC_2.uasf
```
- Alt fixtures: `CLASSIC_6_8.uasf`, or convert a `.sty` with `sff1-parser <file> --uasf-output out.uasf`.
- Select your device with `[` / `]`, confirm with `l`.
- **Pass:** style name + section list print; device selected.

## 2. Per-track split audibility (Gate 10A Task A)
- Press `space` to start, let Main A play.
- **Expected:** distinct instrument parts are audible — drums, bass, chord, and
  at least one melodic/phrase part — NOT a single mushed channel.
- **Score (0–5):** instrument separation = ____

## 3. Intro starts immediately (Gate 10B Task C)
- Press `p` (panic/stop), then `i` (queue intro), then `space` (start).
- **Expected:** the intro sounds on **bar 1** — no full bar of silence before it
  begins (the old one-bar delay must be gone).
- **Pass/Fail:** intro starts on bar 1 = ____

## 4. Chord tracking / NTT (Gate 10A Task B)
- While Main plays, press `c` repeatedly to cycle the chord (C → F → G → Am → …).
- **Expected:** bass follows the chord root; chords re-voice to the new chord
  quality (major vs minor) cleanly, no obviously wrong notes.
- **Score (0–5):** harmonic correctness = ____
- (For precise note-by-note checks use `GATE_10B_NTT_CALIBRATION.md`.)

## 5. Swing / shuffle feel (Gate 10B Task B)
> The CLI plays straight (swing 50) by default. To audition swing, rebuild a
> short test with `pl.setSwing(67)` (triplet) before `start()`, OR confirm the
> unit model in `test_groove` and validate feel here once a swing UI control
> exists. For now, score the **straight** groove tightness.
- **Score (0–5):** groove tightness (straight) = ____
- **Note:** swing factor that feels right for this style = ____ %

## 6. Articulation / keyswitch (Gate 10A Task C / 10B Task E)
- If the device/library uses keyswitches, confirm note attacks change when the
  keyswitch renderer is active; if the device has no keyswitch support, confirm
  graceful degrade (notes still sound, no stuck low keyswitch note).
- **Score (0–5):** articulation behaviour = ____

## 7. Stuck-note / panic safety
- During playback press `p` (PANIC).
- **Expected:** all notes stop immediately; no hanging notes.
- **Pass/Fail:** panic clears all notes = ____

---

## Scorecard (copy into `.openclaw/review/GATE_9_MUSICAL_REVIEW.md` §6)

| Dimension | Score 0–5 | Notes |
|---|---|---|
| Instrument separation (split) | | |
| Intro immediate start | (P/F) | |
| Harmonic correctness (NTT) | | |
| Groove tightness | | |
| Articulation behaviour | | |
| Panic safety | (P/F) | |

**Gate 10B hardware verdict:** PASS / PARTIAL / FAIL — ____
**Device used:** ____   **Date:** ____
