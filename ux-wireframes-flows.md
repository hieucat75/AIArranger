# UX WIREFRAMES & FLOWS
**Live Arranger Platform — v0.9 — Text-based wireframes (landscape iPad primary)**

---

## WIREFRAME 1: MAIN LIVE SCREEN

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ ●●●  Live Arranger                                   🔒 PERF  🎹  ⚙️   9:41 AM │
├──────────────────┬──────────────────────────────────────────────┬───────────────┤
│                  │                                              │               │
│  📁 Cool Shuffle │         C m 7                               │  ═══ MIXER ══ │
│  ─────────────── │    ┌─────────────────────────────────────┐  │               │
│  Setlist         │    │                                     │  │  🥁 Drums  ▐█▌│
│  ┌─────────────┐ │    │         C m 7  /  E                │  │  🎸 Bass   ▐█▌│
│  │▶ 1. Serenade│ │    │    [ chord name — 64pt bold cyan ]  │  │  🎹 Chord  ▐█▌│
│  │  2. My Way  │ │    │                                     │  │  🎹 Pad    ▐█▌│
│  │  3. Unchained│ │   └─────────────────────────────────────┘  │  🎷 Phr1   ▐█▌│
│  │  4. Feelings │ │                                              │  🎷 Phr2   ▐█▌│
│  └─────────────┘ │   ┌───────────┬───────────┬───────────┐     │               │
│                  │   │  INTRO 1  │  INTRO 2  │  INTRO 3  │     │  [COLLAPSE ▶] │
│  [+ NEXT SONG]   │   │   80×64   │   80×64   │   80×64   │     │               │
│                  │   └───────────┴───────────┴───────────┘     │               │
│  ─────────────── │                                              │               │
│  Style           │   ┌───────────┬───────────┬───────────┬─────────────┐       │
│  Cool Shuffle    │   │ ▌MAIN A ▌ │  MAIN B   │  MAIN C   │   MAIN D   │       │
│  ♩ 112 BPM 4/4   │   │ [ACTIVE]  │  [cyan]   │  [default]│  [default] │       │
│                  │   │  ██████   │           │           │            │       │
│  [BROWSE STYLES] │   └───────────┴───────────┴───────────┴─────────────┘       │
│                  │                                              │               │
│                  │   ┌───────────┬───────────┬───────────┬─────────────┐       │
│                  │   │  FILL A   │  FILL B   │  FILL C   │   FILL D   │       │
│                  │   │  [amber]  │  [default]│  [default]│  [default] │       │
│                  │   └───────────┴───────────┴───────────┴─────────────┘       │
│                  │                                              │               │
│                  │   ┌───────────────────┐  ┌─────────────────────────┐        │
│                  │   │      BREAK        │  │  ENDING 1 │ END2 │ END3 │        │
│                  │   │    [default]      │  │  [default] │      │      │        │
│                  │   └───────────────────┘  └─────────────────────────┘        │
│                  │                                              │               │
├──────────────────┴──────────────────────────────────────────────┴───────────────┤
│  [▶ START]  [⏹ STOP]   Bar: 8   Beat: 3   ♩ 112  [-][+]   Xpose: 0  [PANIC🔴] │
└─────────────────────────────────────────────────────────────────────────────────┘

LEGEND:
▌MAIN A▌  = Active section (cyan background, white text)
[FILL A]  = Pending section (amber, pulsing)
[default] = Inactive (Surface2 background)
🔴 PANIC  = 88×88pt minimum, always visible, red
▐█▌       = Mixer fader (vertical slider)
```

**Annotations:**
- Chord display: full width, 80pt height, always above section grid.
- Section grid: 4 columns landscape, equal width, 64pt height minimum.
- Bottom bar: fixed height 56pt, always visible even in performance mode.
- Mixer panel: collapsible, slides từ right, không che section grid khi collapsed.
- Left panel: collapsible, mặc định visible landscape, hidden portrait.

---

## WIREFRAME 2: MIXER SCREEN (Expanded)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  ← LIVE                        MIXER                              [DONE]        │
├─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬──────────┤
│  DRUMS  │  BASS   │ CHORD 1 │ CHORD 2 │  PAD    │  PHR 1  │  PHR 2  │ MASTER   │
│         │         │         │         │         │         │         │          │
│   🥁    │   🎸    │   🎹    │   🎹    │   🎹    │   🎷    │   🎷    │  ⚡      │
│         │         │         │         │         │         │         │          │
│   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃      │
│   ┃     │   ┃     │   █     │   ┃     │   ┃     │   ┃     │   █     │   ┃      │
│   █     │   █     │   █     │   █     │   █     │   █     │   █     │   █      │
│   █     │   █     │   █     │   █     │   █     │   █     │   █     │   █      │
│   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃     │   ┃      │
│         │         │         │         │         │         │         │          │
│  110    │  100    │   95    │   80    │   85    │   90    │   75    │  127     │
│         │         │         │         │         │         │         │          │
│◀ ─●─ ▶ │◀ ─●─ ▶ │◀ ─●─ ▶ │◀ ─●─ ▶ │◀ ─●─ ▶ │◀ ─●─ ▶ │◀ ─●─ ▶ │          │
│  PAN    │  PAN    │  PAN    │  PAN    │  PAN    │  PAN    │  PAN    │          │
│         │         │         │         │         │         │         │          │
│ [MUTE]  │ [MUTE]  │ [MUTE]  │ [MUTE]  │ [MUTE]  │ [MUTE]  │ [MUTE]  │          │
│ [SOLO]  │ [SOLO]  │ [SOLO]  │ [SOLO]  │ [SOLO]  │ [SOLO]  │ [SOLO]  │          │
│         │         │         │         │         │         │         │          │
│   Rev   │   Rev   │   Rev   │   Rev   │   Rev   │   Rev   │   Rev   │  REVERB  │
│  [──●─] │  [─●──] │  [──●─] │  [────] │  [───●] │  [──●─] │  [────] │  [────●] │
│         │         │         │         │         │         │         │          │
│  Cho    │  Cho    │  Cho    │  Cho    │  Cho    │  Cho    │  Cho    │  CHORUS  │
│  [────] │  [────] │  [──●─] │  [────] │  [─●──] │  [────] │  [────] │  [──●──] │
│         │         │         │         │         │         │         │          │
│ USB:9   │ USB:11  │ INT     │ INT     │ INT     │ VIRT:15 │ INT     │          │
│ [ROUTE] │ [ROUTE] │ [ROUTE] │ [ROUTE] │ [ROUTE] │ [ROUTE] │ [ROUTE] │          │
└─────────┴─────────┴─────────┴─────────┴─────────┴─────────┴─────────┴──────────┘

ANNOTATIONS:
- Volume fader: vertical, touch drag. Double-tap to reset to default.
- Pan knob: horizontal drag. Double-tap to center.
- MUTE: toggle, red when muted. 60×44pt.
- SOLO: toggle, yellow when soloed. 60×44pt.
- Reverb/Chorus send: horizontal slider, 0–127.
- ROUTE label: shows current routing (USB:channel / INT = internal / VIRT = virtual).
- Tap ROUTE → opens routing picker sheet.
- Master column: volume only (no pan, no solo, no mute).
```

---

## WIREFRAME 3: STYLE BROWSER

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  ← LIVE                     STYLE LIBRARY                          [IMPORT +]   │
├─────────────────────────────────────────────────────────────────────────────────┤
│  🔍 Search styles...                           [LIST ▼]  [GRID ▦]  [FILTER ⚡]  │
├──────────────────────────────────────────────────────────────────────────────────┤
│  RECENT                                                             [SEE ALL →]  │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐                   │
│  │ Cool       │ │ Latin Pop  │ │ Ballad 8bt │ │ Jazz Waltz │                   │
│  │ Shuffle    │ │ Medium     │ │ 76 BPM     │ │ 3/4        │                   │
│  │ 112 BPM ♥  │ │ 96 BPM    │ │ ⚠ 1 warn  │ │ 120 BPM    │                   │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘                   │
├──────────────────────────────────────────────────────────────────────────────────┤
│  ALL STYLES  (847 total)                    Sort: Last Used ▼                   │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────────┐ │
│  │ ♥  80s Pop Shuffle          Pop, Rock      112 BPM  4/4   SFF1  ▶ PREVIEW  │ │
│  │    [LOAD TO LIVE]                                    ⚠ 1                   │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    Acoustic Ballad          Ballad          72 BPM  4/4   SFF1  ▶ PREVIEW  │ │
│  │    [LOAD TO LIVE]                                                           │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │ ♥  Bossa Nova Modern        Latin           96 BPM  4/4   SFF2  ▶ PREVIEW  │ │
│  │    [LOAD TO LIVE]                                    ⚠ 2                   │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    Cool Jazz Swing          Jazz           132 BPM  4/4   SFF1  ▶ PREVIEW  │ │
│  │    [LOAD TO LIVE]                                                           │ │
│  └─────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                  │
│  [LOAD MORE...]                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘

FILTER SHEET (slides from right):
┌───────────────────────┐
│ FILTER          [DONE]│
│                       │
│ Genre                 │
│ [Pop ✓] [Rock] [Jazz] │
│ [Ballad] [Latin] [...]│
│                       │
│ Tempo                 │
│ [Slow] [Med ✓] [Fast] │
│ Custom: 60──●────160  │
│                       │
│ Format                │
│ [SFF1 ✓] [SFF2 ✓]    │
│                       │
│ Show only:            │
│ [⭐ Favorites]        │
│ [⚠ With warnings]    │
│                       │
│ [CLEAR ALL]  [APPLY]  │
└───────────────────────┘
```

---

## WIREFRAME 4: SETLIST VIEW

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│  ← LIVE                    SETLIST: Show 15/6                   [EDIT] [+ ADD]  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────────┐ │
│  │ ▶  1   NGƯỜI Ở LẠI CHARLIE                              [▶ LOAD]  [···]   │ │
│  │        Cool Shuffle   ♩ 72   Xp: 0   Key: G                                │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    2   MY WAY                                            [▶ LOAD]  [···]   │ │
│  │        Slow Rock 8bt  ♩ 68   Xp: +2  Key: F                                │ │
│  │        Note: Capo 2 nếu dùng guitar                                         │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    3   UNCHAINED MELODY                                  [▶ LOAD]  [···]   │ │
│  │        Slow Rock 8bt  ♩ 64   Xp: 0   Key: G                                │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    4   FEELINGS                                          [▶ LOAD]  [···]   │ │
│  │        Bossa Nova     ♩ 92   Xp: -1  [⚠ Style missing]                   │ │
│  ├─────────────────────────────────────────────────────────────────────────────┤ │
│  │    5   (empty slot)                                      [+ ADD SONG]      │ │
│  └─────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                  │
│  [PRELOAD NEXT ↓]   Currently loaded: Song 1                [CREATE NEW LIST]   │
└─────────────────────────────────────────────────────────────────────────────────┘

SONG DETAIL SHEET (tap [···]):
┌───────────────────────────────┐
│ SONG 2: My Way          [DONE]│
│                               │
│ Title: [My Way          ]     │
│                               │
│ Style: Slow Rock 8bt    [→]   │
│                               │
│ Tempo:  68  BPM  [-][+]      │
│                               │
│ Transpose: +2 [◀][▶]         │
│                               │
│ Notes:                        │
│ [Capo 2 nếu dùng guitar     ] │
│                               │
│ [MOVE UP ↑] [MOVE DOWN ↓]    │
│                               │
│ [REMOVE FROM SETLIST 🗑]     │
└───────────────────────────────┘
```

---

## FLOW 1: IMPORT FLOW

```
[User tap IMPORT +]
        │
        ▼
[Files Picker — multi-select]
        │
        ├── Cancel → back to Library (no change)
        │
        └── Select files (.sty/.prs/.bcs)
                │
                ▼
        [Background Import Begins]
        Toast: "Importing 20 styles..."
        Progress bar in Library tab badge
                │
                ├── [Each file: parse → validate → write .uas]
                │
                ▼
        [Import Complete]
        ┌─────────────────────────────────────┐
        │ ✅ Import Complete                  │
        │                                     │
        │ 18 styles imported successfully     │
        │ 2 imported with warnings            │
        │ 0 failed                            │
        │                                     │
        │ [VIEW WARNINGS]    [DONE]           │
        └─────────────────────────────────────┘
                │
        ┌───────┴──────────┐
        │                  │
        ▼                  ▼
[VIEW WARNINGS]         [DONE]
        │              → Library (styles visible)
        ▼
┌──────────────────────────────────────────────┐
│ IMPORT WARNINGS                        [DONE]│
│                                              │
│ Cool Shuffle.sty                             │
│  ℹ OTS Settings skipped (not supported)     │
│                                              │
│ Genos Modern.sty                             │
│  ⚠ Guitar NTR: mapped to Root Transposition │
│  ℹ OTS Settings skipped                     │
│                                              │
│ [These styles will still play correctly      │
│  for most musical contexts.]                 │
└──────────────────────────────────────────────┘

FAIL CASES:
┌─────────────────────────────────────┐
│ ❌ Import Failed                    │
│                                     │
│ corrupted_file.sty                  │
│ Invalid SMF header. File may be     │
│ corrupted or unsupported format.    │
│                                     │
│ [TRY AGAIN]     [SKIP THIS FILE]   │
└─────────────────────────────────────┘
```

---

## FLOW 2: STYLE LOAD TO LIVE FLOW

```
[User in Style Browser — tap LOAD TO LIVE]
        │
        ▼
[Is arranger PLAYING?]
        │
    ┌───┴───┐
   YES      NO
    │        │
    ▼        ▼
┌──────────┐  [Load immediately]
│ ⚠ Dialog │       │
│          │       ▼
│ Loading  │  [Style loads — < 2s]
│ new style│       │
│ will stop│       ▼
│ playback.│  [Live Screen: style name updated]
│          │  [Section reset to Main A]
│[CANCEL]  │  [Sound init sequence sent to MIDI out]
│[CONTINUE]│
└──────────┘
        │
    [CONTINUE]
        │
        ▼
  [Stop transport]
  [Load new style]
        │
        ▼
  [Live Screen ready]
```

---

## FLOW 3: ERROR HANDLING UX

### Error Categories & Presentation

```
SEVERITY 1 — INFO (không interrupt)
  Presentation: Toast notification, bottom of screen, 3s auto-dismiss
  Example: "OTS Settings không được hỗ trợ trong phiên bản này"
  Action: None required

SEVERITY 2 — WARNING (acknowledge)
  Presentation: Banner dưới chord display, tap để xem detail
  Example: "Guitar voicing đã được điều chỉnh cho 2 tracks"
  Action: Tap để đọc, swipe để dismiss

SEVERITY 3 — ERROR (action required)
  Presentation: Modal sheet, block interaction
  Example: "Import thất bại: SMF header không hợp lệ"
  Action: Must tap to dismiss; offer retry or alternative

SEVERITY 4 — CRITICAL (emergency)
  Presentation: Full overlay (không modal — không dismiss được)
  Example: "Audio session bị ngắt kết nối" (in future)
  Action: Specific recovery action required
```

### Error Messages — Tone & Format

```
✅ ĐÚNG:
  "2 tracks sẽ dùng Root Transposition thay Guitar voicing.
   Style vẫn phát đúng nhạc trong hầu hết trường hợp."

  → Mô tả cụ thể điều gì xảy ra
  → Giải thích impact thực tế
  → Không dùng technical jargon (CASM, Ctb2, NTR) với user

❌ SAI:
  "SFF2_GUITAR_NTR_DEGRADED on tracks 4,6"
  "Error code 0x1F3A"
  "An unexpected error occurred"
```

### MIDI Device Disconnect

```
[Device disconnects during show]
        │
        ▼
[Transport continues — không dừng]
        │
        ▼
[Banner màu đỏ xuất hiện dưới chord display:]
┌──────────────────────────────────────────────────────────┐
│ ⚠ MIDI Device "Roland SC-8850" disconnected              │
│ Sound switched to internal.   [RECONNECT]  [DISMISS]     │
└──────────────────────────────────────────────────────────┘
        │
        ├── [RECONNECT] → check device → nếu found → init sequence → resume
        │
        └── [DISMISS] → banner biến mất, internal sound tiếp tục
```

### Panic UX

```
[User tap PANIC]
        │
        ▼ (no confirm dialog — emergency action)
[All notes off: < 50ms]
        │
        ▼
[Brief red flash trên toàn màn hình: 150ms]
        │
        ▼
[Toast: "All notes off" — 1.5s]
        │
        ▼
[Transport tiếp tục nếu đang chơi]
[Section state giữ nguyên]
```
