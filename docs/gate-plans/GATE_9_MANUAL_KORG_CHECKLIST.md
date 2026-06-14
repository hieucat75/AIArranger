# Gate 9 — Manual Korg Test Checklist (for PTH)

> Hardware: Korg PA (or any external MIDI arranger). Connect via USB/MIDI **before** running.
> Tool: `korg-playback` (Task E). Each step = one command or one action, with the
> expected behavior and the pass/fail signal. Record scores at the end → copy into
> `.openclaw/review/GATE_9_MUSICAL_REVIEW.md` §6.

The CLI takes an optional UASF file argument; destination is chosen live with `[` / `]`.

```
Usage:  korg-playback [style.uasf]      # no arg = built-in demo style
Live keys: space=start/stop  1..6=section  i=intro  f=fill  e=ending
           c=cycle chord  p=PANIC  [ / ]=prev/next dest  l=list  ?=help  q=quit
```

---

## 1. Build
```
cd /Users/pth/Developer/AIArranger/build
cmake --build . --target korg-playback
```
- **Expected:** builds with no errors; produces `build/korg-playback`.
- **Pass:** binary exists.

## 2. Run (with a converted UASF fixture)
```
./korg-playback ../corpus/yamaha/sff1/CLASSIC_6_8.uasf
```
- Alt (demo, no file): `./korg-playback`
- Alt (other fixture): `./korg-playback ../corpus/yamaha/sff1/POP_ACOUSTIC_2.uasf`
- **Expected:** prints style name + section list, then a numbered list of MIDI destinations.
- **Pass:** your Korg appears in the destination list.

## 3. Select MIDI destination
- Press `l` → lists destinations (selected marked `*`).
- Press `]` / `[` → move selection to your Korg.
- **Expected:** `[dest] -> [n] <Korg name>` printed.
- **Pass:** selected line shows the Korg; `hasLiveDestination` true (sent>0 after start).

## 4. Play
- Press `space` to start.
- **Expected:** Korg starts sounding the style; status line shows `state=Intro/Main`, `sent` rising, `stuck=0`.
- **Pass:** audio comes out of the Korg; `dropped` stays ~0 while a device is selected.

## 5. Start / Stop / Panic
- `space` (stop) → **Expected:** sound stops, no hanging notes (`stuck=0`).
- `space` (start again) → resumes.
- `p` (PANIC) mid-phrase → **Expected:** instant silence, `state=Panic`, `active-notes=0`.
- **Pass:** no stuck/hanging notes after stop or panic.

## 6. Section navigation
- `i` intro, `1`–`6` sections, `f` fill, `e` ending.
- **Expected:** section change happens **on the next bar boundary** (not mid-bar); fill is one bar then returns.
- **Pass:** transitions land on the bar; no hung notes across switches (`stuck=0`).

## 7. Chord changes
- Press `c` repeatedly to cycle: C → G → Am → F → Dm → Em → (C).
- **Expected:** accompaniment transposes to each chord; re-voicing is clean (engine flushes notes on chord change).
- **Pass:** pitches follow the chord (root/fifth correct); no stuck notes after each change.

## 8. Score groove + articulation (0–5 each)
Listen across a full cycle and rate:

| Category | Score (0–5) | Notes |
|----------|:-----------:|-------|
| Groove preservation (feel/swing) |  |  |
| Articulation fidelity (keyswitch/MegaVoice) |  |  |

- On quit (`q`) the CLI prints final `NoteBalance` (**expect `stuck=0`**) and schedule→dispatch latency.
- **Action:** put these two scores into `GATE_9_MUSICAL_REVIEW.md` §6 (currently 17/25);
  recompute TOTAL. **FULL PASS** when TOTAL ≥ 15 and no category < 2 with the manual scores filled in.

## 9. Quit
```
q
```
- **Expected:** `[latency] ... Final balance: stuck=0 ... Bye.` then clean exit.
- **Pass:** `stuck=0` in the final report.
