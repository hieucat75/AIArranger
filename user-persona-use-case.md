# USER PERSONA & USE CASE DOCUMENT
**Live Arranger Platform — v0.9**

---

## 1. PERSONA CHÍNH

### Persona 1 — Minh (Nhạc công tiệc cưới chuyên nghiệp)

**Demographics:** Nam, 35 tuổi, TP.HCM. Chơi keyboard 15 năm. Thu nhập chính từ biểu diễn đám cưới, tiệc công ty, nhà hàng. Trung bình 3–4 show/tuần.

**Setup hiện tại:**
- Yamaha PSR-SX900 (cây chính).
- Roland Sound Module (SC-8850) cho sound phụ.
- iPad Pro 12.9" (dùng để xem chords + notes, đôi khi Camelot Pro).
- Thư viện: ~800 style .sty, hầu hết download từ group Facebook + tự edit.

**Goals:**
- Bỏ bớt thiết bị nặng. Hiện tại phải mang: keyboard + stand + sound module + amp = 25kg+.
- Quản lý setlist nhanh hơn (hiện tại dùng file Excel trên điện thoại).
- Tìm style nhanh hơn khi khách request bài bất ngờ.

**Pain points:**
- PSR-SX900 style browser chậm và khó tìm khi đang diễn.
- Phải về nhà mới edit setlist được (không edit trên sân khấu).
- Một số style download về không chạy đúng trên máy (version mismatch).
- Không có cách backup/restore nếu máy hỏng giữa tour.

**Tech comfort:** Cao. Biết cài app, configure MIDI. Không cần hướng dẫn cơ bản.

**Quote:** "Tôi chỉ cần cái gì đó chạy đúng trên sân khấu. Không cần fancy, cần stable."

---

### Persona 2 — Hương (Nhạc công nhà thờ)

**Demographics:** Nữ, 42 tuổi, Hà Nội. Chơi organ/keyboard tại nhà thờ 20 năm. Không chuyên nghiệp hoàn toàn, có công việc khác nhưng nhạc là passion.

**Setup hiện tại:**
- Yamaha PSR-E460 (mid-range). Muốn lên đời nhưng chưa có budget.
- 150 style download từ internet.

**Goals:**
- Có thêm style chất lượng cao hơn machine hiện tại.
- Quản lý bài hát cho thánh lễ mỗi tuần.
- Học cách dùng công nghệ để âm nhạc tốt hơn.

**Pain points:**
- PSR-E460 âm thanh không đủ tốt cho nhà thờ lớn.
- Khó tìm style phù hợp cho thánh ca.
- Không rành kỹ thuật MIDI.

**Tech comfort:** Trung bình. Dùng iPhone thành thạo. Cần onboarding rõ ràng.

**Quote:** "Nếu dễ dùng và giúp buổi lễ hay hơn, tôi sẵn sàng trả tiền."

---

### Persona 3 — Carlos (Nhạc công Filipino, semi-pro)

**Demographics:** Nam, 29 tuổi, Manila. Chơi keyboard ở bar + studio gigs. Genos user.

**Setup hiện tại:**
- Yamaha Genos (full). Cây đắt nhất trên thị trường.
- Thư viện 2,000+ style (mix built-in + download + mua).
- MacBook Pro cho studio work.

**Goals:**
- Dùng iPad khi gig nhỏ (không muốn mang Genos đi mọi nơi).
- Tìm app iOS có thể chạy Genos style với chất lượng chấp nhận được.
- Thử nghiệm công nghệ mới — là early adopter.

**Pain points:**
- Không có iOS app nào dùng được Genos style.
- vArranger không chạy trên iPad.
- Apple không có ecosystem arranger.

**Tech comfort:** Rất cao. Engineer background. Sẵn sàng beta test và báo bug chi tiết.

**Quote:** "I'll pay good money for something that actually works. $50 one-time is nothing compared to a $4,000 arranger."

---

## 2. USE CASES — CHI TIẾT

### UC-01: Import và chuẩn bị style cho show (Minh)

**Pre-condition:** Minh có 20 file .sty cần dùng tối nay.
**Trigger:** Minh mở app lúc 3pm, 4 tiếng trước show.

**Flow chính:**
1. Minh tap "Import Style" → Files picker mở.
2. Navigate đến iCloud Drive / "Style Library" folder.
3. Select 20 file → tap "Import".
4. App chạy background import. Minh browse style đã có trong khi chờ.
5. Notification: "20 styles imported. 2 warnings." Tap để xem.
6. 2 warning: "OTS skipped" (info) + "Guitar NTR degraded" (warning). Minh đọc, hài lòng.
7. Minh search "ballad" → filter → tìm 5 style cần dùng → favorite.
8. Tạo setlist "Show 15/6" → add 12 songs → assign style + tempo per song.
9. Save setlist.

**Post-condition:** Setlist sẵn sàng, 12 song đã assign style.
**Duration target:** < 10 phút cho toàn bộ flow (20 style + 12 song setlist).

---

### UC-02: Live performance — show đang chạy (Minh)

**Pre-condition:** App đang chạy, setlist "Show 15/6" loaded. MIDI keyboard kết nối USB.
**Trigger:** MC announce bài đầu tiên.

**Flow chính:**
1. Minh tap "Start" → Count-in 2 bar → Main A bắt đầu.
2. Tay trái: bấm hợp âm theo bài. App nhận diện, style theo.
3. Điệp khúc: tap "Main B" → pending indicator nhấp nháy → cuối bar: Main B commit.
4. Cuối bài: tap "Ending 1" → ending chạy → auto stop.
5. Khách request bài lạ. Minh tap "Style Browser" → search → load style mới trong < 3 giây.
6. Bài mới: tap "Intro 1" → start.
7. Giữa show, sound bị stuck. Minh tap Panic (đỏ to) → tất cả note tắt ngay.
8. Cuối show: tap "Next Song" → song kế preload → load ngay.

**Post-condition:** Show kết thúc, không crash, không lệch nhịp.
**Edge case:** MIDI keyboard rút ra → app cảnh báo nhưng không dừng, fallback sang internal sound.

---

### UC-03: Chuẩn bị thánh lễ (Hương)

**Pre-condition:** Hương chưa bao giờ dùng app. Vừa download.
**Trigger:** Hương mở app lần đầu.

**Flow onboarding (không trong scope MVP nhưng UX phải consider):**
1. Onboarding screen: "Import style của bạn hoặc chơi ngay với sound có sẵn."
2. Hương chọn "Import" → hướng dẫn tìm file trên iPhone/Files.
3. Import 5 style → success.
4. Tutorial overlay: "Đây là section buttons. Tap để chuyển."
5. Hương không có MIDI keyboard → dùng On-screen Chord Pad.
6. Tap C major → style chạy. Ngạc nhiên và thích.

**Flow hàng tuần:**
1. Mở app, chọn setlist "Thánh lễ CN".
2. Kiểm tra style + tempo từng bài.
3. Thực hành: chơi 1–2 lần, điều chỉnh tempo.
4. Sunday: dùng trong lễ.

---

### UC-04: Multi-MIDI setup cho show lớn (Carlos)

**Pre-condition:** Carlos có Genos + iPad + Roland JD-800. Gigs ở venue lớn.
**Setup:** iPad → USB MIDI → JD-800 (pad/strings). iPad → Virtual MIDI → GarageBand (xử lý thêm). Genos: vẫn cắm cho drums (internal sound Genos).

**Flow:**
1. App: routing "Pad track → channel 1 → USB-JD800". "Drums → internal sound". "Phrase → Virtual MIDI".
2. Load Genos style file (SFF2).
3. App parse, warn "Guitar NTR degraded" → Carlos chấp nhận.
4. Tap Start → iPad phát pad/phrase; Carlos chơi tay phải melody qua Genos.
5. Chord từ tay trái trên Genos → USB MIDI in → app nhận chord → style thay đổi.

**Post-condition:** Hybrid setup hoạt động, âm thanh tốt hơn iPad standalone.

---

## 3. USER JOURNEY MAP — MINH (Persona 1)

```
Giai đoạn    Nhận biết → Thử → Mua → Dùng hàng ngày → Recommend

Trigger      Thấy video YouTube "iPad thay arranger keyboard"
             Tìm kiếm "yamaha style iOS app"

Actions      → Download app (free)
             → Import 5 style (free tier)
             → Thử tại nhà
             → Mua Pro ($29.99)
             → Import toàn bộ 800 style
             → Dùng trong show thật
             → Post lên Facebook group
             → Recommend cho 3 người bạn

Emotions     Tò mò → Ngạc nhiên (style chạy được!) → Lo lắng (có đủ stable không?)
             → Tin tưởng (2 show không crash) → Hào hứng → Evangelist

Pain points  Không chắc chắn về quality → Giải quyết: video demo + beta reviews
trong        Không biết style mình có chạy không → Giải quyết: free import 5 style trước khi mua
journey      Sợ crash giữa show → Giải quyết: panic button + 2h stability guarantee

Opportunities
for delight  - Import xong hiện "847 styles ready" → "Wow, tất cả đều import được!"
             - First chord → style plays → "Nghe đúng hơn tôi tưởng!"
             - Section switch smooth → "OK cái này professional rồi"
```

---

## 4. JOBS TO BE DONE

Nhìn theo framework JTBD (Jobs to be Done):

| Job | Persona | Frequency | Importance |
|---|---|---|---|
| "Chơi show tối nay không mang nhiều thiết bị" | P1 | 3–4x/tuần | Critical |
| "Tìm style phù hợp cho bài khách request trong 30 giây" | P1, P3 | Mỗi show | High |
| "Quản lý setlist 12 bài cho lễ Chủ nhật" | P2 | Mỗi tuần | High |
| "Backup toàn bộ style library an toàn" | P1, P3 | 1x/tháng | Medium |
| "Thử Genos style mới trước khi mua" | P3 | 1–2x/tháng | Medium |
| "Dạy học sinh chord progression với visual feedback" | P2 | 1x/tuần | Low |
