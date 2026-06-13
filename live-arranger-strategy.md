# LIVE ARRANGER PLATFORM — TÀI LIỆU PHẢN HỒI CHIẾN LƯỢC
**Phiên bản 1.0 — Phản hồi Master Brief**

---

## 1. EXECUTIVE SUMMARY

**Quyết định cốt lõi (trả lời 10 câu hỏi chiến lược):**

| # | Câu hỏi | Quyết định |
|---|---------|-----------|
| 1 | iPad-only hay macOS-first? | **iPad-first.** macOS là phase 2 (engine C++ dùng chung). |
| 2 | Dùng JUCE core? | **Có, nhưng chỉ ở tầng engine.** UI thuần SwiftUI. JUCE làm audio/MIDI/DSP backbone trong C++ core. |
| 3 | MIDI output trước internal sound engine? | **Đúng. MIDI output là MVP.** Internal sound (sfizz + GM soundfont) là "nice-to-have" cuối MVP. |
| 4 | Scope import Yamaha basic? | **SFF1 đầy đủ + SFF2 đọc được phần MIDI cơ bản** (bỏ qua GE/Mega Voice ở MVP). Parse CASM/Ctab, NTR/NTT, tất cả section. |
| 5 | Expansion Pack nghiên cứu mức nào? | **Research-only, không implement.** Pack thương mại bị mã hóa — tuyệt đối không bypass. Thay bằng user sample import (SFZ/SF2/WAV). |
| 6 | Internal format: JSON, binary hay hybrid? | **Hybrid container:** file `.uas` (zip) chứa `manifest.json` (metadata, chord rules) + MIDI events dạng SMF chunks + samples folder. |
| 7 | Realtime scheduler chống jitter? | **Audio-clock-driven, lookahead scheduling 2 buffer, lock-free queue.** Chi tiết ở mục 5. |
| 8 | Rủi ro pháp lý import style user-owned? | **Thấp và quản lý được.** Parse format để tương thích là hợp pháp (tiền lệ interoperability). Rủi ro thật nằm ở trademark, bundled content và DRM — đã có chính sách rõ ở mục 8. |
| 9 | Build-vs-buy? | Ma trận đầy đủ ở mục 6. Tóm tắt: **build** arranger/chord/style-parser (core IP), **buy/reuse** sampler/DSP/MIDI-stack. |
| 10 | Roadmap 6 tháng? | Sprint 0 (4 tuần) → MVP TestFlight beta cuối tháng 6. Chi tiết mục 9. |

**Tóm tắt định vị:** Không có sản phẩm nào trên iPad hiện nay vừa import style Yamaha tốt, vừa có UX live-first hiện đại, vừa có kiến trúc mở. vArranger mạnh về tương thích nhưng UX cũ và là Windows-centric; One Man Band UX lỗi thời; Camelot Pro/MainStage không phải arranger. Khoảng trống thị trường rõ ràng: **"Loopy Pro của thế giới arranger"** — đó là sản phẩm chúng ta xây.

---

## 2. PRODUCT VISION

**Tuyên bố tầm nhìn:** Biến iPad thành một arranger workstation mở — nơi người chơi keyboard live mang theo toàn bộ thư viện style, sound và setlist của họ, không bị khóa vào phần cứng của một hãng.

**3 trụ cột:**
1. **Compatibility moat** — đọc được style người dùng đã sở hữu (Yamaha trước), càng nhiều format càng giá trị.
2. **Live-first UX** — mọi quyết định thiết kế phục vụ người đang đứng trên sân khấu, không phải người ngồi studio.
3. **Open platform** — internal format trung lập + plugin interface + pack metadata chuẩn hóa ngay từ đầu, để marketplace và AI layer là phần mở rộng tự nhiên chứ không phải đập đi xây lại.

**Người dùng mục tiêu MVP (persona chính):** Nhạc công keyboard chơi tiệc cưới/nhà hàng/nhà thờ, đã sở hữu kho style Yamaha (.STY), có MIDI keyboard hoặc sound module, muốn bỏ bớt cây arranger nặng nề. Đây là persona có pain point mạnh nhất và sẵn sàng trả tiền nhất.

---

## 3. MVP SCOPE

### 3.1. Trong MVP (phải có)
- iPad app (iPadOS 16+), portrait + landscape, tối ưu landscape.
- Import Yamaha .STY/.PRS/.BCS/.SST — SFF1 đầy đủ; SFF2 đọc track MIDI chuẩn (degrade gracefully phần không hỗ trợ, báo rõ).
- Parse đủ section: Intro 1–3, Main A–D, Fill A–D (gồm Fill In BA/AB...), Break, Ending 1–3.
- Parse CASM: Ctab, NTR (Root Trans/Root Fixed), NTT (Bypass/Melody/Chord/Bass/Melodic Minor/Harmonic Minor...), chord mute, note limit, retrigger rule.
- Chord recognition realtime: Fingered + Single Finger, split point, bass inversion/slash chord, hold chord. 14 loại hợp âm như brief.
- Realtime arranger engine: sync start/stop, quantized section switch (fill = beat-quantized, section = bar-quantized theo hành vi Yamaha), auto fill on/off, tap tempo, count-in, panic.
- MIDI output: per-track channel, PC/Bank MSB-LSB, CC, chọn output device (CoreMIDI: USB/BLE/Network/Virtual), MIDI clock out.
- Mixer cơ bản: volume/pan/mute/solo per track, master volume.
- Style browser + tag/search/favorite.
- Setlist đơn giản: song → style + tempo + transpose + snapshot, next-song preload.
- Performance save/load.
- **Internal sound "good enough":** sfizz + 1 GM soundfont chất lượng khá, để app dùng được standalone không cần sound module. Đây là ranh giới: có, nhưng không đầu tư sâu.

### 3.2. Ngoài MVP (cố tình bỏ)
- Korg/Roland/Ketron import. Ketron audio style (audio loop) đặc biệt phức tạp — phase 3.
- Yamaha Expansion Pack (.ppi/.cpi) — chỉ research.
- AUv3 hosting (cám dỗ lớn nhưng tốn 1–2 tháng; phase 2).
- Marketplace, cloud, AI.
- Drum kit editor, multisample engine nâng cao.
- Lyrics/chord chart hiển thị.
- Style recording/editing (chỉ playback).

### 3.3. Tiêu chí thành công MVP
- 20 nhạc công beta dùng được trong show thật ≥ 2 giờ không crash.
- ≥ 90% style SFF1 phổ biến import và chơi đúng (test corpus 500 style).
- Chord-to-sound latency < 20ms (MIDI out), < 30ms (internal sound, buffer 128 @ 44.1kHz).
- Section switch sai nhịp = 0 trong điều kiện CPU bình thường.

---

## 4. ARCHITECTURE OVERVIEW

```
┌─────────────────────────────────────────────┐
│  SwiftUI App (UI, navigation, library mgmt) │
│  - Live Screen / Mixer / Browser / Setlist  │
└──────────────────┬──────────────────────────┘
                   │ Swift ↔ C++ bridge (Swift/C++ interop, Xcode 15+)
┌──────────────────▼──────────────────────────┐
│  C++ Realtime Core (JUCE engine layer)      │
│  ┌─────────────┐ ┌──────────────┐           │
│  │ Style Engine │ │ Chord Engine │           │
│  │ (UASF model) │ │ (lookup+rule)│           │
│  └──────┬──────┘ └──────┬───────┘           │
│  ┌──────▼─────────────── ▼──────┐           │
│  │   Arranger Engine            │           │
│  │   (state machine + NTT       │           │
│  │    transform + quantize)     │           │
│  └──────┬───────────────────────┘           │
│  ┌──────▼───────────────────────┐           │
│  │   Scheduler (audio-clock,    │           │
│  │   lookahead, lock-free queue)│           │
│  └──┬──────────────────────┬────┘           │
│  ┌──▼─────────┐   ┌────────▼────────┐       │
│  │ MIDI Engine │   │ Sound Engine    │       │
│  │ (router)    │   │ (sfizz + DSP    │       │
│  │             │   │  graph + mixer) │       │
│  └──┬─────────┘   └────────┬────────┘       │
└─────┼──────────────────────┼────────────────┘
┌─────▼─────┐         ┌──────▼──────┐
│ CoreMIDI  │         │ CoreAudio / │
│           │         │ AVAudioEngine│
└───────────┘         └─────────────┘
```

**Nguyên tắc kiến trúc then chốt:**
1. **UI không bao giờ chạm audio thread.** Mọi lệnh từ UI đi qua lock-free command queue; engine trả state về UI qua atomic snapshot, UI poll 60Hz.
2. **Style Import Adapter là offline process** — chạy background thread/queue riêng, output là UASF, validate xong mới vào library. Runtime engine chỉ biết UASF, không biết Yamaha.
3. **Sound Engine là plugin phía sau MIDI router** — internal sound, external MIDI, hybrid chỉ là cấu hình routing, không phải code path khác nhau.
4. **AI/Marketplace là service interface** đã khai báo từ đầu (`IStyleProcessor`, `IPackProvider`) nhưng chưa có implementation.

---

## 5. MODULE BREAKDOWN — QUYẾT ĐỊNH & TRADE-OFF

### Module 2+3 — Style Import & Universal Internal Style Format (UASF)

**Định dạng `.uas` = zip container:**
```
style.uas
├── manifest.json     # metadata, version, sections index,
│                     # chord rules (NTR/NTT/mute/limit) per track per section,
│                     # sound mapping, drum mapping, FX hints
├── sections/
│   ├── main_a.mid    # SMF format 0, tick-accurate, đã tách theo section
│   ├── fill_a.mid
│   └── ...
└── samples/          # (phase 2) sample references cho user pack
```

**Lý do hybrid thay vì JSON thuần hoặc binary thuần:**
- MIDI events ở JSON sẽ phình 10–20× và parse chậm; SMF là binary chuẩn, tooling sẵn, debug bằng bất kỳ DAW nào.
- Metadata/chord rules ở JSON: human-readable, dễ migrate version, dễ cho creator tools sau này.
- Zip container: 1 file = 1 style, dễ share, dễ marketplace, có chỗ cho samples.
- Versioning: `formatVersion` trong manifest + migration code; section MIDI không bao giờ đổi format (SMF bất biến).

**Mapping Yamaha → UASF (điểm khó nhất):**
- SFF1 Ctab → UASF chord rules: ánh xạ 1:1 được.
- SFF2 Ctb2 có low/mid/high range riêng → UASF cần hỗ trợ multi-range rule ngay từ schema v1 (kể cả khi MVP chỉ dùng 1 range) để không phải migrate đau đớn.
- Tham khảo JJazzLab (mã nguồn mở, Java) — đã reverse-engineer SFF1/SFF2 khá đầy đủ. **Học thuật toán, không copy code (license LGPL, ta dùng C++ tự viết).**

### Module 4 — Realtime Arranger Engine (trả lời câu 7: scheduler)

**Thiết kế chống jitter — 4 quyết định:**
1. **Clock = audio callback.** Sample position của audio render là nguồn thời gian duy nhất. Tick = samples × (tempo/60) × PPQ / sampleRate. Không dùng timer OS, không dùng `DispatchQueue`.
2. **Lookahead scheduling:** mỗi audio callback, arranger render trước các MIDI event trong cửa sổ [now, now + 2×buffer]. Event được timestamp theo sample frame → CoreMIDI gửi với `MIDITimeStamp` tương lai (host time), nội bộ sound engine nhận sample-accurate offset trong buffer.
3. **Chord change = realtime path riêng:** khi user bấm hợp âm, các note đang vang của track pitched được xử lý theo retrigger rule (stop/pitch-shift/retrigger theo Ctab) ngay tại event đó, không đợi lookahead window — đây là chỗ mọi arranger app dở bị "trễ hợp âm".
4. **Section switch quantize:** lệnh từ UI vào lock-free queue → arranger commit tại boundary (fill: beat kế tiếp; main/ending: bar kế tiếp — đúng hành vi Yamaha). Trạng thái "đã đặt lệnh, chờ boundary" phải hiển thị nhấp nháy trên UI.

### Module 5 — Chord Engine
Build tay, nhỏ (~2 tuần): interval lookup table cho 14+ chord types, ưu tiên match nhiều note nhất, bass note thấp nhất = inversion/slash. Single Finger theo convention Yamaha (root / root+trắng dưới = 7 / root+đen dưới = minor...). Thiết kế dạng pure function `(notes[], mode) → Chord` để unit test dày đặc.

### Module 6 — MIDI Engine
CoreMIDI trực tiếp (không qua JUCE MIDI wrapper trên iOS — CoreMIDI cho timestamp scheduling tốt hơn). Hỗ trợ USB, BLE MIDI, Network Session, Virtual MIDI (để app khác như AUM nhận được — đây là feature cộng đồng iOS music rất cần, gần như free).

### Module 7 — Sound Engine
- **sfizz (BSD-2)** cho SFZ + convert SF2→SFZ lúc import. Tránh FluidSynth trên iOS (LGPL dynamic-link rắc rối với App Store).
- DSP graph: JUCE `AudioProcessorGraph` hoặc graph tự viết mỏng; FX từ JUCE DSP module + Airwindows (MIT) cho reverb/comp chất lượng tốt miễn phí.
- Phase 2 mới làm: velocity layers UI, round robin, drum kit editor, disk streaming.

### Module 8 — Expansion Pack (trả lời câu 5)
Phase đầu **chỉ làm research deliverable**: tài liệu format tổng quan, phân loại phần nào đọc được hợp pháp (metadata không mã hóa) vs phần nào cấm (sample data mã hóa, license-bound vào instrument ID). Kết luận trước: **con đường đúng là user sample import + AI-assisted remap (phase 2), không phải đọc .ppi.**

### Module 10+11 — Mixer & Live UI
- Live screen: section pad grid (giống layout panel arranger nhưng thiết kế lại touch-first, không clone), chord display lớn, bar/beat indicator, tempo/transpose, 1 hàng mixer mini, panic luôn hiển thị.
- Performance mode: khóa mọi gesture điều hướng, chỉ còn nút live + double-tap để mở khóa.
- Mọi nút section ≥ 60×60pt, phản hồi < 1 frame, trạng thái queued/active/next phân biệt bằng màu + animation.

### Module 14+15 — AI & Marketplace
MVP chỉ làm 2 việc rẻ: (1) pack metadata schema có đủ author/license/version/dependency/ownership ngay từ file `.uas` đầu tiên; (2) interface `IStyleProcessor` trong core để AI sau này cắm vào như một import post-process step có logging diff.

---

## 6. BUILD VS BUY MATRIX

| Module | Quyết định | Giải pháp | Lý do / Trade-off |
|---|---|---|---|
| App Shell / UI | **Build** | SwiftUI | Không có lựa chọn khác tốt hơn; UI là differentiator. |
| Style Parser (Yamaha) | **Build** | C++ tự viết, học từ JJazzLab | Core IP. Không có thư viện C++ thương mại nào. |
| UASF | **Build** | JSON + SMF + zip | Trái tim của platform, phải sở hữu 100%. |
| Arranger Engine | **Build** | C++ thuần | Core IP, không tồn tại off-the-shelf. |
| Chord Engine | **Build** | C++ thuần | Nhỏ, 2 tuần, cần kiểm soát hành vi từng edge case. |
| Audio/MIDI backbone | **Buy/Reuse** | JUCE (license Indie ~$40/th nếu doanh thu <$500k; lên Pro sau) | Tiết kiệm 3–6 tháng. Trade-off: phí license, binary size. Thay thế: tự viết trên CoreAudio — chỉ làm nếu muốn bỏ hẳn cross-platform. |
| MIDI I/O iOS | **Reuse native** | CoreMIDI trực tiếp | Timestamp scheduling + BLE/Network tốt nhất. |
| Sampler | **Reuse OSS** | sfizz (BSD-2) + TinySoundFont (MIT) làm fallback SF2 | Tránh viết sampler từ zero (6+ tháng). Trade-off: phải tự thêm streaming phase 2. |
| FX/DSP | **Reuse OSS + JUCE** | JUCE DSP, Airwindows (MIT), zita-rev1 port | Chất lượng đủ live. Phase 3 mới cân nhắc SDK thương mại (2C Audio, Relab...). |
| Time-stretch (Ketron audio style, phase 3) | **Buy** | Rubber Band (commercial license) hoặc Signalsmith Stretch (MIT) | Chưa cần MVP. |
| Database/Library | **Reuse** | SQLite + GRDB (Swift) | Chuẩn, ổn. |
| AI layer | **Hoãn** | ONNX Runtime / CoreML phase 2+ | Chỉ chốt interface. |
| Cloud/Marketplace | **Hoãn** | — | Chỉ chốt metadata schema. |

---

## 7. TECHNICAL RISKS

| # | Rủi ro | Mức | Giảm thiểu |
|---|---|---|---|
| T1 | SFF2 phức tạp hơn dự kiến (Ctb2 multi-range, GE) → import sai nhạc tính | Cao | Test corpus 500+ style thật ngay Sprint 0; định nghĩa "degrade gracefully": chơi được phần MIDI chuẩn, báo rõ phần bỏ qua. |
| T2 | Jitter/glitch khi CPU load (import nền + chơi live) | Cao | Audio-clock scheduler (mục 5), import ở QoS thấp, đo bằng stress test tự động trong CI (chạy style + đổi section 1000 lần, assert timing). |
| T3 | Swift↔C++ bridge gây overhead/bug | Vừa | Dùng Swift/C++ interop chính thức (Xcode 15+), API surface mỏng: command queue vào, state snapshot ra. |
| T4 | BLE MIDI latency/dropout trên sân khấu | Vừa | Khuyến nghị USB trong app; hiển thị cảnh báo latency khi dùng BLE; panic button luôn hoạt động kể cả mất MIDI device. |
| T5 | sfizz thiếu tính năng cho GM bank chất lượng | Vừa | Chọn soundfont GM tốt + chấp nhận "good enough" MVP; sound chất lượng cao là phase 2 với multisample engine. |
| T6 | JUCE license/AppStore conflict | Thấp | JUCE 7+ đã ổn trên iOS App Store; mua license Indie từ đầu. |
| T7 | Team thiếu kinh nghiệm realtime audio | Cao nếu xảy ra | Vị trí bắt buộc: 1 senior C++ audio engineer. Không thỏa hiệp vị trí này. |

---

## 8. LEGAL RISKS (trả lời câu 8)

| # | Rủi ro | Đánh giá | Chính sách |
|---|---|---|---|
| L1 | Parse format .STY của file user sở hữu | **Thấp.** Reverse-engineer format để interoperability có tiền lệ pháp lý vững (Sega v. Accolade, Sony v. Connectix, Google v. Oracle - US; EU Software Directive Art.6). File .STY user-owned do user import, app không phân phối. | Cho phép. Ghi rõ trong ToS: user chịu trách nhiệm quyền sở hữu file import. |
| L2 | Bypass mã hóa Expansion Pack (.ppi/.cpi) | **Cấm tuyệt đối.** Vi phạm DMCA §1201 / tương đương EU. | Không implement, không tài liệu hóa cách bypass, kể cả "research". |
| L3 | ROM samples Yamaha/Korg | **Cấm tuyệt đối.** | Không bundle, không hướng dẫn extract. Marketplace sau này có quy trình kiểm duyệt sample ownership. |
| L4 | Trademark | **Vừa — rủi ro thật nhất.** | Chỉ dùng tên hãng dạng nominative: "imports styles in Yamaha® .STY format". Không dùng trong tên app, icon, screenshot ASO. Disclaimer "not affiliated". |
| L5 | Bundle style thương mại | Cấm. | App ship với style tự sản xuất/commission + CC0. |
| L6 | User upload nội dung vi phạm (marketplace, phase 3) | Hoãn nhưng chuẩn bị | DMCA agent + takedown process trong legal policy draft trước khi mở marketplace. |

**Deliverable pháp lý Sprint 0:** IP Risk Matrix chi tiết + ToS draft + trademark usage guideline — nên review bởi luật sư IP thật trước khi launch public, ngân sách ~$3–5k.

---

## 9. ROADMAP 6 THÁNG

**Sprint 0 — Tuần 1–4 (Foundation & De-risk)**
- Thu thập test corpus 500+ style Yamaha (SFF1/SFF2, nhiều đời máy).
- Spike 1: parse 1 file .STY → dump section/CASM → phát Main A qua CoreMIDI. *Đây là spike quyết định — nếu 2 tuần không xong, xem lại team.*
- Spike 2: JUCE audio loop + sfizz phát GM soundfont, đo latency thật trên iPad.
- Chốt UASF spec v0.9 + Technical Architecture Doc v1.
- Setup CI + timing stress test harness.

**Tháng 2–3 (Engine Core)**
- Style Import Adapter SFF1 hoàn chỉnh + SFF2 basic, validation + error report.
- Arranger Engine: playback, NTT transform, retrigger rules, section state machine, quantized switching, sync start/stop, auto fill.
- Chord Engine + unit test toàn bộ chord table.
- Milestone M3: *chơi live bằng MIDI keyboard → style phát đúng qua MIDI out, đổi section đúng nhịp.* Demo nội bộ với 2–3 nhạc công.

**Tháng 4 (I/O & Sound)**
- MIDI routing đầy đủ (PC/Bank/CC, device selection, virtual MIDI, clock out).
- Internal sound: sfizz + GM bank, mixer engine, reverb/chorus send.
- Latency tuning, panic, MIDI disconnect handling.

**Tháng 5 (Product)**
- Live Screen + Mixer UI + Performance mode.
- Style browser, library (SQLite), tag/search.
- Setlist + performance snapshot + next-song preload.
- Import flow UX + error handling UX.

**Tháng 6 (Beta)**
- TestFlight closed beta 20 nhạc công (ưu tiên cộng đồng VN + quốc tế).
- 2 vòng feedback, stability (mục tiêu: 2 giờ live không crash), polish.
- Chuẩn bị App Store listing (đã qua trademark review).

**Sau tháng 6 (định hướng):** AUv3 hosting → user sample/SFZ import UI → SFF2 nâng cao → Korg adapter → macOS (Catalyst hoặc native AppKit shell trên cùng engine).

**Team tối thiểu:**
- 1 Senior C++ realtime audio engineer (full-time, bắt buộc).
- 1 iOS/SwiftUI engineer (full-time).
- 1 Product/domain expert là nhạc công arranger thật (founder hoặc part-time).
- 1 Designer (part-time, tập trung tháng 4–5).
- QA part-time từ tháng 4 + beta community.

---

## 10. REQUIRED DOCUMENTS LIST (trạng thái & thứ tự ưu tiên)

**Ưu tiên 1 — cần trước khi code (Sprint 0):**
1. Universal Internal Style Format Spec ⬅ quan trọng nhất
2. Technical Architecture Document
3. Realtime Arranger Engine Spec (gồm scheduler design)
4. File Import Strategy (Yamaha SFF1/SFF2 mapping table + limitation list)
5. Legal/IP Risk Assessment
6. MVP Scope Document (chốt từ mục 3 tài liệu này)

**Ưu tiên 2 — song song tháng 2–3:**
7. Chord Recognition Engine Spec
8. MIDI Engine Spec
9. Sound Engine Spec (phase 1)
10. Data Model + Performance & Latency Requirement
11. PRD + User Persona & Use Case
12. Testing Strategy (gồm style corpus test plan)

**Ưu tiên 3 — tháng 4–5:**
13. UX Concept + Wireframes (Live/Mixer/Browser/Setlist/Import/Error)
14. BRD + Competitive Analysis (bản đầy đủ)
15. Expansion Pack Research Report
16. Build-vs-Buy Matrix (bản chi tiết từ mục 6) + Risk Register + Open Questions

---

## 11. NEXT ACTION (2 tuần tới)

1. **Phê duyệt 10 quyết định chiến lược** ở Executive Summary — đặc biệt #1 (iPad-first), #2 (JUCE), #6 (hybrid format). Mọi tài liệu tiếp theo phụ thuộc vào đây.
2. **Bắt đầu thu thập style test corpus** — việc không cần engineer, làm ngay được.
3. **Tuyển/xác nhận senior C++ audio engineer** — bottleneck lớn nhất của toàn dự án.
4. **Yêu cầu tài liệu tiếp theo:** tôi đề xuất viết **UASF Spec v0.9** trước (schema JSON đầy đủ + example + mapping table Yamaha→UASF), vì nó unblock cả parser lẫn engine. Sau đó đến Realtime Arranger Engine Spec.
5. **Mua JUCE Indie license + đăng ký Apple Developer** nếu chưa có.

