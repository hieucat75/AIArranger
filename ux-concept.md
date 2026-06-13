# UX CONCEPT
**Live Arranger Platform — v0.9**

---

## 1. TRIẾT LÝ THIẾT KẾ

### "Sân khấu trước, studio sau"

Mọi quyết định UX xuất phát từ một câu hỏi: **"Nhạc công đang đứng trên sân khấu tối, tay đang chơi, có thể thao tác được không?"**

Nếu câu trả lời là "cần nhìn kỹ" hoặc "cần dừng chơi để làm" — thiết kế lại.

### 5 nguyên tắc cốt lõi

**P1 — Tap one, hear now.** Mọi thao tác quan trọng (section switch, mute, tempo) phải là 1 tap, phản hồi ngay. Không confirm dialog, không slide menu.

**P2 — Glance-readable.** Nhạc công nhìn màn hình < 0.5 giây trong khi chơi. Chord, section đang phát, tempo — phải đọc được từ 1m xa trong điều kiện ánh sáng yếu.

**P3 — Fat finger safe.** Mọi interactive target ≥ 60×60pt. Spacing giữa các nút nguy hiểm (Ending / Panic) ≥ 20pt.

**P4 — Errors are not catastrophes.** Bấm nhầm section → có thể cancel (pending state). Bấm Panic → confirm không cần (emergency). Style không load → clear message, không crash.

**P5 — Performance mode is sacred.** Khi bật performance mode: không có gesture nào điều hướng ra khỏi Live Screen. Chỉ live controls hoạt động.

---

## 2. VISUAL LANGUAGE

### 2.1. Color System

```
Background:    #0D0D0F   (near-black, dễ nhìn trên sân khấu tối)
Surface:       #1A1A1F   (card, panel)
Surface2:      #252530   (elevated surface)

Accent:        #00E5FF   (cyan — active state, chord display)
AccentWarm:    #FFB300   (amber — pending/queued state)
Danger:        #FF3B30   (red — panic only)
Success:       #30D158   (green — recording, active good state)
Muted:         #636370   (disabled, inactive)

Text primary:  #FFFFFF
Text secondary:#A0A0B0
Text tertiary: #606070
```

Lý do chọn cyan làm accent: dễ nhìn trong điều kiện dim lighting; không confuse với đỏ (danger) hay xanh lá (success); phân biệt tốt với amber (pending).

### 2.2. Typography

```
Display (chord name):     SF Pro Rounded Bold, 64pt
Header (section label):   SF Pro Display Semibold, 24pt
Body (tempo, bar):        SF Pro Text Medium, 17pt
Caption:                  SF Pro Text Regular, 13pt
Monospace (MIDI values):  SF Mono, 13pt
```

SF Pro Rounded cho chord display: ký tự nhạc (♭♯) render đẹp, dễ đọc to.

### 2.3. Component States

Mỗi interactive element có 5 state với visual rõ ràng:

| State | Visual |
|---|---|
| **Default** | Surface2 background, Text secondary label |
| **Active** | Accent background (#00E5FF), White label, subtle glow |
| **Pending** | AccentWarm background (#FFB300), pulsing animation 0.8Hz |
| **Pressed** | Scale 0.95, darken 15% — haptic feedback |
| **Disabled** | Muted background, Muted label, no interaction |

### 2.4. Spacing & Sizing

```
Grid unit:         8pt
Minimum tap target: 60×60pt (live controls), 44×44pt (secondary)
Panic button:      88×88pt (minimum)
Section pad:       80×64pt (landscape), 60×48pt (portrait)
Chord display:     Full width, 80pt height
Corner radius:     12pt (cards), 8pt (buttons), full (pills)
```

### 2.5. Animation

```
Section switch commit:   scale 1.0→1.05→1.0, 150ms ease-in-out
Pending pulse:           opacity 1.0→0.6→1.0, 600ms loop
Chord change:            cross-fade text, 80ms
Panel slide:             0.3s spring (damping 0.8)
Panic press:             immediate, no animation (emergency)
```

Không dùng animation > 300ms cho live controls — delay cảm thấy là bug với nhạc công.

---

## 3. LAYOUT SYSTEM

### 3.1. Landscape (primary)

```
┌──────────────────────────────────────────────────────┐
│  STATUS BAR (thin, không chiếm không gian)           │
├──────────┬───────────────────────────────────────────┤
│          │                                           │
│  LEFT    │           CENTER                          │
│  PANEL   │         LIVE AREA                         │
│  (Nav)   │                                           │
│  200pt   │                                           │
│          │                                           │
├──────────┴──────────────────────────┬────────────────┤
│      BOTTOM BAR                     │  RIGHT PANEL   │
│      (Chord + Tempo + Transport)    │  (optional)    │
└─────────────────────────────────────┴────────────────┘
```

Left Panel: Style name + setlist navigation (collapsible).
Center Live Area: Section pads + chord display (không bao giờ collapse).
Bottom Bar: Luôn visible — chord, tempo, transport.
Right Panel: Mixer mini (collapsible) hoặc hidden.

### 3.2. Portrait (secondary)

```
┌────────────────────────┐
│  Chord Display         │  ← 100pt
├────────────────────────┤
│  Section Pads          │  ← 240pt
├────────────────────────┤
│  Quick Controls        │  ← 80pt
│  (Tempo, Transpose)    │
├────────────────────────┤
│  Mini Mixer (collapsed)│  ← 60pt
└────────────────────────┘
```

Portrait: ít section pad hiển thị hơn (scroll để xem Intro/Ending). Chủ yếu Main A–D + Fill luôn visible.

### 3.3. Navigation Model

```
Tab Bar (ẩn trong performance mode):
  🎵 Live         ← default tab, không bao giờ có back button
  📚 Library      ← Style browser
  📋 Setlist
  ⚙️  Settings

Modal sheets (swipe down to dismiss):
  - Import flow
  - Style detail
  - Performance editor

Full-screen overlays (performance mode safe):
  - Mixer (slide from right)
  - Keyboard on-screen (slide from bottom)
```

Quy tắc: **Live tab không bao giờ có navigation stack**. Mọi thứ từ Live mở ra là modal hoặc side panel, không push navigation. Nhạc công swipe down để đóng, luôn quay về Live.

---

## 4. INTERACTION PATTERNS

### 4.1. Section Pad Interaction

```
Single tap:
  → Nếu không có pending: set pending, highlight amber
  → Nếu có pending cùng loại: cancel pending, về default
  → Nếu có pending khác: replace pending

Long press (> 0.5s):
  → Loop mode cho section đó (vd lock Main A không auto-advance)
  → Visual: thêm lock icon

Double tap:
  → Mở section options (volume, transpose riêng — phase 2)
```

### 4.2. Chord Display

```
Tap chord display:
  → Mở on-screen chord pad (slide up)

Long press chord display:
  → Toggle hold mode (lock icon xuất hiện)

Swipe left/right on chord display:
  → Transpose +1 / -1 semitone
```

### 4.3. Tempo Control

```
Tap tempo number:
  → Edit mode, numeric keyboard
  → Confirm: tap elsewhere hoặc Return

Tap "TAP" button:
  → Tap tempo (4 tap lấy BPM)

Long press "TAP":
  → Reset về style default tempo

Swipe up/down on tempo:
  → +1 / -1 BPM per swipe unit (velocity sensitive)
```

### 4.4. Performance Mode

```
Activate:   3-finger tap anywhere trên Live Screen
            hoặc tap lock icon ở corner
Deactivate: 3-finger tap lại
            hoặc double-tap lock icon

Khi active:
  - Tab bar ẩn
  - Navigation gestures disabled
  - Swipe-from-edge disabled
  - Chỉ live controls (section pads, panic, tempo, chord) active
  - Lock icon visible ở corner
```

---

## 5. HAPTIC FEEDBACK

| Action | Haptic |
|---|---|
| Section pad tap (set pending) | Light impact |
| Section commit (boundary crossed) | Medium impact |
| Panic | Heavy impact (immediate) |
| Tempo tap | Selection |
| Mute toggle | Light impact |
| Error | Error pattern (double tap) |
| Import complete | Success notification |

Haptic phải sync với visual. Không haptic nếu action không thành công.

---

## 6. ACCESSIBILITY (basic — phase 1)

- VoiceOver labels cho mọi button (tên section, function).
- Dynamic Type: tối thiểu hỗ trợ "Large" size.
- High Contrast mode: không dùng màu làm phương tiện thông tin duy nhất (luôn kèm icon hoặc label).
- Không dựa vào gesture phức tạp cho chức năng core.

---

## 7. ONBOARDING (MVP — tối giản)

Lần đầu mở app:
1. Screen: "Import style để bắt đầu" + button lớn + skip option.
2. Skip → vào Live Screen với demo style (built-in, không import).
3. Overlay hint: 3 tooltip cho section pad, chord pad, panic — dismiss by tap.
4. Không có wizard dài dòng.

**Triết lý onboarding:** nhạc công là người dùng experienced. Không giải thích "arranger là gì". Giải thích chỉ những gì khác với hardware (section pad interaction, split point).
