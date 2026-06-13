# BRD — BUSINESS REQUIREMENTS DOCUMENT
**Live Arranger Platform — v0.9**

---

## 1. EXECUTIVE SUMMARY

**Cơ hội:** Thị trường nhạc công arranger keyboard (Yamaha PSR/Genos, Korg Pa series) có hàng triệu người dùng toàn cầu. Không có sản phẩm phần mềm iOS nào hiện tại kết hợp đủ ba yếu tố: import được style library người dùng đã sở hữu, UX live-first hiện đại, và kiến trúc mở cho ecosystem. Khoảng trống này là cơ hội để xây dựng platform category-defining.

**Giải pháp:** iOS/iPadOS live arranger platform — cho phép nhạc công keyboard mang toàn bộ setup (style, sound, setlist) trong iPad, không phụ thuộc vào phần cứng cụ thể.

**Mục tiêu kinh doanh 12 tháng:**
- TestFlight beta Q2 2026 với 20 nhạc công.
- App Store launch Q3 2026.
- 1,000 paying users trong 6 tháng sau launch.
- MRR $5,000 trong 6 tháng sau launch (break-even team nhỏ).
- Foundation cho marketplace revenue stream (phase 3, năm 2).

---

## 2. THỊ TRƯỜNG MỤC TIÊU

### 2.1. Thị trường tổng (TAM)
- Toàn cầu: ~15–20 triệu nhạc công keyboard sử dụng arranger keyboard (ước tính từ doanh số Yamaha PSR/SX/Genos + Korg Pa series hàng năm × vòng đời sản phẩm 7–10 năm).
- Có iPhone/iPad: ~60–70% (profile nhạc công professional/semi-pro).
- TAM: ~10 triệu người.

### 2.2. Thị trường có thể tiếp cận (SAM)
- Nhạc công có style library Yamaha đã sở hữu + đang dùng iPad: ~1–2 triệu.
- Nhạc công chuyên nghiệp và semi-pro (sẵn sàng trả tiền cho tool chất lượng): ~200,000–500,000.

### 2.3. Thị trường thực tế (SOM) — Year 1
- Target: 5,000–10,000 users (paid).
- Conservative: 1,000 users sau 6 tháng từ launch.

### 2.4. Địa lý ưu tiên
- **Phase 1:** Việt Nam (thị trường nhạc công tiệc cưới/nhà hàng lớn, iPad adoption cao, early adopter base của founder), Philippines, Indonesia.
- **Phase 2:** Toàn Đông Nam Á + cộng đồng Yamaha style quốc tế (forum PSR-S700.com, yamahamusicians.com, v.v.).
- **Phase 3:** US, EU, Japan.

---

## 3. PERSONA KINH DOANH

### P1 — Nhạc công live chuyên nghiệp (Core, sẵn sàng trả tiền nhất)
- **Profile:** 28–50 tuổi, chơi keyboard tại tiệc cưới/nhà hàng/nhà thờ, thu nhập từ biểu diễn.
- **Setup hiện tại:** Yamaha PSR-S970/SX900/Genos + sound module (thường Roland/Korg).
- **Pain point:** Phải mang thiết bị nặng; style library 500+ files khó quản lý; muốn iPad thay thế hoặc bổ sung.
- **WTP (Willingness to Pay):** $10–15/tháng hoặc $50–80 one-time (so với $2,000–5,000 cho hardware arranger).
- **Kênh tiếp cận:** Facebook group nhạc công, YouTube tutorials, word-of-mouth.

### P2 — Nhạc công studio / content creator
- **Profile:** 25–45 tuổi, làm nhạc nền, sản xuất, cần nhanh.
- **Pain point:** Không muốn cắm keyboard vật lý, cần demo nhanh arrangement.
- **WTP:** $10–20/tháng.
- **Kênh:** YouTube, Reddit r/WeAreTheMusicMakers.

### P3 — Nhạc công nghiệp dư nâng cao
- **Profile:** 40–65 tuổi, đã có Yamaha PSR tầm trung, muốn thêm tính năng.
- **Pain point:** Hardware arranger cũ không có feature mới; muốn style của người khác mà không mua máy mới.
- **WTP:** $5–10/tháng; nhạy cảm giá hơn P1.
- **Kênh:** Forum Yamaha style, YouTube hướng dẫn.

---

## 4. COMPETITIVE LANDSCAPE

### 4.1. Đối thủ trực tiếp

| Sản phẩm | Platform | Strengths | Weaknesses |
|---|---|---|---|
| **vArranger 3** | Windows | Format support tốt nhất (Yamaha/Korg/Roland); được cộng đồng tin dùng | Windows only; UX cũ (Win32); không mobile; không live-first |
| **One Man Band** | Windows/macOS | Mature, nhiều tính năng | UX rất cũ; interface phức tạp; không iOS; ít update |
| **JJazzLab** | Desktop (Java) | Open source; SFF1/2 tốt | Java app nặng; không mobile; không live-focused |
| **iSymphonic Orchestra** | iOS | iOS native | Không phải arranger; không import style |

### 4.2. Đối thủ gián tiếp

| Sản phẩm | Overlap | Điểm khác |
|---|---|---|
| Camelot Pro | Live performance management iOS | Không có arranger engine; dùng AUv3/MIDI |
| MainStage (Apple) | Live keyboard performance macOS | Không có arranger; chỉ macOS |
| Loopy Pro | Live performance iOS | Loop-based, không arranger |
| AUM | iOS audio host | Không có arranger; chỉ là host |

### 4.3. Khoảng trống thị trường

Không có sản phẩm nào trên iOS/mobile kết hợp:
✅ Import Yamaha style library của user.
✅ UX live-first, touch-first, sân khấu.
✅ MIDI output + internal sound.
✅ Kiến trúc mở (format chuẩn, plugin-ready, marketplace-ready).

Đây là "white space" rõ ràng. Sản phẩm nào làm tốt 3 điều trên trước sẽ chiếm vị trí category leader.

---

## 5. MÔ HÌNH KINH DOANH

### 5.1. Revenue streams (theo phase)

**Phase 1 — MVP (Tháng 1–9):**
- **Freemium:** App free, import 5 style miễn phí. Trả phí để unlock full.
- **One-time purchase:** $29.99 hoặc $39.99 (one-time "Pro unlock").
- **Lý do:** Lowering barrier to try; nhạc công thích one-time hơn subscription.

**Phase 2 (Tháng 10–18):**
- **Subscription tier:** $9.99/tháng hoặc $79.99/năm ("Pro Plus") — thêm tính năng nâng cao (AUv3 hosting, advanced sound engine, cloud sync setlist).
- Giữ one-time tier cho core features.

**Phase 3 — Marketplace (Năm 2+):**
- **Commission 30%** trên style/sound pack bán qua marketplace.
- **Creator Pro subscription:** $19.99/tháng cho creator tools (nếu có).
- **Enterprise/band license:** giá đặc biệt cho studio, school.

### 5.2. Unit economics (conservative estimate)

```
Year 1 target: 1,000 paying users
Average revenue per user: $35 (one-time purchase weighted average)
Year 1 revenue: ~$35,000 gross ($24,500 sau 30% App Store cut)
→ Không đủ break-even team full-time; cần bootstrapping hoặc pre-seed.

Year 2 target: 5,000 users (mix one-time + subscription)
ARPU: $45
Year 2 revenue: ~$225,000 gross ($157,500 net)
→ Đủ sustain team 2–3 người + tiếp tục đầu tư.

Marketplace breakeven thêm: nếu GMV marketplace = $50k/tháng × 30% = $15k/tháng phí.
```

### 5.3. Cost structure (Year 1)

```
Engineering (2 FTE equivalent): $80,000–120,000
Design (part-time): $10,000–15,000
Legal (trademark + ToS): $5,000
Infrastructure (CI, Crashlytics, cloud): $2,000
JUCE license + tools: $2,000
Marketing + beta: $5,000
─────────────────────────────────
Total Year 1 burn: ~$105,000–150,000
Funding needed: Pre-seed $150,000–200,000 hoặc bootstrapped với consulting income.
```

---

## 6. YÊU CẦU KINH DOANH

### BR-01: Interoperability làm "moat"
App phải import được style library nhạc công đã sở hữu — đây là lý do switch cost sang platform khác rất cao sau khi user đã import 500 style.

### BR-02: Thời gian ra sản phẩm
MVP TestFlight phải sẵn sàng trong 6 tháng. Mỗi tháng delay = mất cơ hội xây community trước khi competitor (nếu có) nhận ra khoảng trống.

### BR-03: App Store compliant từ đầu
Không có tính năng nào vi phạm App Store Review Guideline. Không có game plan "launch và sửa sau khi bị reject" — reject có thể mất 1–2 tháng.

### BR-04: Viral loop từ community
Style pack là shareable content. Thiết kế flow "export style list" hoặc "share setlist" từ đầu — để nhạc công giới thiệu cho nhau là cách marketing tự nhiên nhất.

### BR-05: Pricing không cannibalize hardware
Không marketing kiểu "thay thế Yamaha Genos" — cần hardware arranger đắt tiền vẫn mua instrument. Marketing kiểu "bổ sung" hoặc "mang studio trong túi" ít gây friction với nhà sản xuất hơn.

---

## 7. THÀNH CÔNG KHI NÀO

| Milestone | Metric | Thời hạn |
|---|---|---|
| Product-market fit signal | ≥ 10 nhạc công dùng trong show thật, tự nguyện recommend | Tháng 7 |
| Beta success | ≥ 18/20 beta user pass 2-giờ live test | Tháng 6 |
| Launch readiness | App Store approval, zero critical bugs | Tháng 8 |
| Revenue traction | 100 paying users trong tháng đầu sau launch | Tháng 9 |
| Growth signal | 30% user acquisition via word-of-mouth | Tháng 12 |
| Platform signal | ≥ 3 creator muốn upload pack lên marketplace | Tháng 15 |
