# PRD — PRODUCT REQUIREMENTS DOCUMENT
**Live Arranger Platform — iOS MVP — v0.9**

---

## 1. PRODUCT OVERVIEW

**Tên làm việc:** Live Arranger (tên thật pending trademark search)
**Platform:** iPadOS 16+, Universal (iPhone support secondary)
**Target launch:** App Store Q3 2026

**One-liner:** iPad app cho nhạc công keyboard — import style Yamaha, chơi live, không cần mang phần cứng arranger.

---

## 2. USER STORIES — MVP

### Epic 1: Import Style

**US-01** — Import style Yamaha cơ bản
```
Là nhạc công có style Yamaha trên Files app,
Tôi muốn import file .sty vào app,
Để có thể dùng style đó trong buổi diễn.

Acceptance criteria:
  - Import từ Files app (iCloud Drive, On My iPad, USB).
  - Hỗ trợ .sty, .bcs, .prs.
  - SFF1: import thành công ≥ 95% file phổ biến.
  - SFF2: import thành công ≥ 75%, degrade gracefully phần không hỗ trợ.
  - Hiển thị progress indicator trong khi import (không block UI).
  - Sau import: hiển thị danh sách warning/limitation rõ ràng.
  - Nếu lỗi chặn: thông báo rõ lý do, không import silent corrupt.
  - Import chạy background; user có thể tiếp tục dùng app trong khi chờ.
```

**US-02** — Import nhiều style cùng lúc
```
Là nhạc công có thư viện 500+ style,
Tôi muốn import nhiều file một lúc,
Để không phải import từng cái.

Acceptance criteria:
  - Multi-select trong Files picker.
  - Batch import với progress overall (vd "Importing 47/120...").
  - Lỗi một file không dừng batch.
  - Log lỗi từng file, summary sau khi xong.
```

**US-03** — Xem thông tin style sau import
```
Acceptance criteria:
  - Tên, genre, tempo, time signature, sections có sẵn.
  - Danh sách limitations với severity icon (info/warning).
  - Preview: phát Main A 4 bar qua MIDI out hoặc internal sound.
```

### Epic 2: Live Playback

**US-04** — Chơi style realtime qua MIDI keyboard
```
Là nhạc công đang diễn,
Tôi muốn bấm hợp âm trên keyboard → style phát theo,
Để thay thế cây arranger hardware.

Acceptance criteria:
  - MIDI keyboard cắm USB: hoạt động ngay, không cần cấu hình thêm.
  - Chord recognition < 20ms từ bấm đến style phản hồi (internal sound < 30ms).
  - Fingered mode và Single Finger mode, chuyển đổi không interrupt.
  - Split point cấu hình được.
  - Chord display lớn, rõ trên sân khấu.
```

**US-05** — Chuyển section đúng nhịp
```
Acceptance criteria:
  - Nút section lớn (≥ 60×60pt), phản hồi visual tức thì.
  - Main A→B→C→D: commit cuối bar, pending indicator nhấp nháy.
  - Fill: commit cuối beat.
  - Ending: commit cuối bar, chạy hết → stop.
  - Không lệch nhịp trong bất kỳ điều kiện CPU bình thường.
  - Nếu đã đặt pending, nhấn section khác → thay pending (không queue nhiều).
```

**US-06** — Tempo control
```
Acceptance criteria:
  - Tempo display lớn, tap để edit.
  - Tap Tempo: 4 tap lấy trung bình, cập nhật ngay.
  - Tempo range: 20–300 BPM.
  - Tempo change không gây glitch audio.
  - Transpose: -12 đến +12 semitone, cập nhật ngay.
```

**US-07** — Panic button
```
Acceptance criteria:
  - Luôn hiển thị, không bị che.
  - Minimum touch target 80×80pt.
  - Tap → tất cả note tắt trong < 50ms.
  - Gửi Note Off + CC123 + CC120 trên mọi channel.
  - Transport tiếp tục (không dừng style).
```

**US-08** — MIDI output
```
Acceptance criteria:
  - Chọn MIDI output device (USB, BLE, Network, Virtual).
  - Per-track channel configuration.
  - Program Change/Bank sent tự động khi load style.
  - MIDI clock out on/off.
  - Hiển thị cảnh báo nếu chọn BLE (latency warning).
```

### Epic 3: Mixer

**US-09** — Mixer cơ bản
```
Acceptance criteria:
  - Volume slider per track (0–127).
  - Pan per track.
  - Mute button per track (lớn, rõ trên sân khấu).
  - Solo per track.
  - Master volume.
  - Thay đổi có hiệu lực ngay, không delay.
  - Mixer state lưu trong Performance.
```

### Epic 4: Library & Style Browser

**US-10** — Duyệt style library
```
Acceptance criteria:
  - List + grid view.
  - Search theo tên (real-time, < 200ms).
  - Filter theo genre, tag.
  - Sort: tên, ngày import, last used.
  - Favorite toggle.
  - Recently used section.
  - Tap style → preview option (không load ngay vào live).
```

**US-11** — Load style vào live
```
Acceptance criteria:
  - Tap "Load to Live" → style load (< 2 giây).
  - Nếu đang chơi: warning "Loading new style will stop playback. Continue?"
  - Sau load: style mới sẵn sàng, section reset về Main A.
  - Style name hiển thị trên Live Screen.
```

### Epic 5: Setlist & Performance

**US-12** — Tạo setlist
```
Acceptance criteria:
  - Tạo setlist, đặt tên.
  - Thêm song: assign style + tempo + transpose + mixer snapshot.
  - Reorder bằng drag.
  - Next song preload: khi đang chơi song N, song N+1 load sẵn trong RAM.
  - Chuyển song: "Next Song" button lớn → không interrupt nếu style chưa stop.
```

**US-13** — Performance save/load
```
Acceptance criteria:
  - Save: tên + style + tempo + transpose + mixer state + routing + chord engine config.
  - Load: tất cả settings restore trong < 1 giây.
  - Danh sách performance có search.
  - Export/import performance file (.json) để backup.
```

### Epic 6: On-screen Chord Pad

**US-14** — Chơi không cần MIDI keyboard
```
Acceptance criteria:
  - Grid root × type, nút đủ lớn (≥ 48×48pt).
  - Tap root → giữ, tap type → phát chord.
  - Chord phát ngay, không delay.
  - Chord display cập nhật.
  - Dùng được song song với MIDI keyboard (không conflict).
```

---

## 3. FEATURE PRIORITY MATRIX

| Feature | Priority | Sprint |
|---|---|---|
| Import SFF1 style | P0 | 1 |
| Arranger engine + NTT transform | P0 | 1–2 |
| Chord recognition (fingered + single finger) | P0 | 1 |
| MIDI output (external device) | P0 | 1–2 |
| Section switching (quantized) | P0 | 2 |
| Live Screen UI | P0 | 3 |
| Panic button | P0 | 2 |
| Internal sound (sfizz + GM) | P1 | 3 |
| Mixer cơ bản | P1 | 3 |
| Style browser + search | P1 | 3 |
| Setlist + performance save | P1 | 3–4 |
| On-screen chord pad | P1 | 4 |
| Tap tempo | P1 | 2 |
| SFF2 basic import | P1 | 2–3 |
| MIDI clock out | P2 | 3 |
| BLE MIDI support | P2 | 3 |
| Virtual MIDI port | P2 | 3 |
| Batch import | P2 | 4 |
| Import SFF2 advanced (guitar NTR, etc.) | P3 | Post-MVP |
| AUv3 hosting | P3 | Phase 2 |
| AI remap | P3 | Phase 2 |
| Korg/Roland import | P3 | Phase 2 |
| Marketplace | P3 | Phase 3 |

---

## 4. NON-FUNCTIONAL REQUIREMENTS

| ID | Requirement | Metric | Priority |
|---|---|---|---|
| NFR-01 | Stability | 0 crash trong 2h live session | P0 |
| NFR-02 | Chord latency | < 20ms MIDI out; < 30ms internal sound | P0 |
| NFR-03 | Section switch timing | 0 sai nhịp trong điều kiện bình thường | P0 |
| NFR-04 | Startup time | < 5 giây từ tap icon đến sẵn sàng | P1 |
| NFR-05 | Import time | < 5 giây / style (< 500KB) | P1 |
| NFR-06 | Battery | < 15% drain / giờ (screen on, MIDI out, no internal sound) | P1 |
| NFR-07 | Library scan | 500 style scan metadata < 10 giây (background) | P2 |
| NFR-08 | UI responsiveness | Mọi tap response < 100ms visual feedback | P1 |
| NFR-09 | Localization | Tiếng Anh MVP; Tiếng Việt phase 2 | P2 |
| NFR-10 | Accessibility | VoiceOver basic support trên UI controls | P3 |

---

## 5. UX REQUIREMENTS

**Màn hình chính (Live Screen) — must have:**
- Section pad: Intro 1/2/3, Main A/B/C/D, Fill A/B/C/D, Break, Ending 1/2/3.
- Chord display: font lớn ≥ 48pt, luôn visible.
- Tempo + Transpose: editable inline.
- Bar/beat indicator.
- Style name.
- Panic button: luôn visible, không bị overlay che.
- Quick mixer (1 row, volume per track).
- Quick access: style browser, setlist.

**Performance mode:**
- Bật: khóa navigation gestures, chỉ live controls active.
- Tắt: double-tap hoặc dedicated button.
- Mục đích: tránh bấm nhầm mở menu trong show.

**Visual design principles:**
- Dark mode mặc định (dễ nhìn trên sân khấu tối).
- Contrast ratio ≥ 4.5:1 trên mọi text.
- Active state: màu accent rõ (neon green/blue — không dùng màu yếu).
- Pending state: nhấp nháy chậm (0.8Hz), không gây nhức mắt.
- Không dùng màu đỏ cho state bình thường (đỏ = warning/panic only).

---

## 6. PLATFORM REQUIREMENTS

- **iOS minimum:** iPadOS 16.0.
- **Device:** iPad (all sizes). iPhone: không tối ưu MVP nhưng không crash.
- **Landscape orientation primary.** Portrait: UI reflow tối giản.
- **Split View / Stage Manager:** support basic (không crash, layout thay đổi gracefully).
- **External display:** phase 2 (second screen cho audience / setlist view).
- **Apple Pencil:** không có requirement MVP.

---

## 7. OUT OF SCOPE (MVP)

Những thứ bị hỏi — câu trả lời là KHÔNG cho MVP:
- Style editor / recording.
- DAW integration / AUv3.
- Cloud sync.
- Multi-user / collaboration.
- Score/chord chart display.
- Drum machine / pattern editor.
- AI generation.
- Korg/Roland style import.
- macOS.
- Android/Windows.
- Marketplace.
