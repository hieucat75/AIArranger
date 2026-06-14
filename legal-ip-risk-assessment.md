# LEGAL & IP RISK ASSESSMENT
**Live Arranger Platform — Trạng thái: Draft — Cần review bởi luật sư IP trước launch**

---

## 1. TUYÊN BỐ PHẠM VI

Tài liệu này phân tích rủi ro pháp lý và sở hữu trí tuệ của dự án theo 6 hạng mục: (1) format compatibility & reverse engineering, (2) sample & sound licensing, (3) trademark, (4) DSP/code licensing, (5) user-generated content, (6) marketplace. Mỗi rủi ro có: đánh giá mức độ, căn cứ pháp lý, chính sách hành động, và deliverable cần hoàn thành trước milestone tương ứng.

**Disclaimer nội bộ:** Tài liệu này là phân tích kỹ thuật-pháp lý do team dự án soạn, không phải tư vấn pháp lý chính thức. Ngân sách dự kiến cho luật sư IP review: **$3,000–5,000 USD** trước App Store submission. Ưu tiên: trademark search + ToS review.

---

## 2. IP RISK MATRIX TỔNG QUAN

| # | Lĩnh vực rủi ro | Mức rủi ro | Tác động nếu xảy ra | Khả năng xảy ra |
|---|---|---|---|---|
| L1 | Reverse-engineer format .STY (user-owned file) | **Thấp** | Kiện tụng civil | Rất thấp |
| L2 | Bypass DRM / mã hóa Expansion Pack | **Cấm tuyệt đối** | DMCA criminal + civil | N/A — không làm |
| L3 | Bundle Yamaha/Korg ROM sample | **Cấm tuyệt đối** | Copyright infringement | N/A — không làm |
| L4 | Trademark (tên app, marketing) | **Vừa — rủi ro thật nhất** | Buộc đổi tên / takedown | Trung bình nếu không cẩn thận |
| L5 | JUCE license vi phạm | **Thấp** | Mất quyền dùng JUCE | Thấp nếu mua đúng tier |
| L6 | Open-source license vi phạm | **Vừa** | Forced open-source / injunction | Thấp nếu audit đúng |
| L7 | Soundfont / SFZ pack licensing | **Thấp–Vừa** | Copyright claim | Thấp nếu chọn đúng |
| L8 | User import nội dung vi phạm | **Vừa** | Secondary liability | Trung bình (mitigate bằng ToS) |
| L9 | Marketplace — creator upload vi phạm | **Cao (phase 3)** | DMCA takedown / platform liability | Cao nếu không có DMCA process |
| L10 | Patent (audio DSP algorithm) | **Thấp** | Injunction | Rất thấp (dùng open algo) |

---

## 3. PHÂN TÍCH CHI TIẾT

### L1 — Reverse Engineering Format .STY

**Câu hỏi pháp lý:** Có được phép parse file .STY mà user sở hữu hợp pháp không?

**Phân tích:**

Căn cứ pháp lý ủng hộ việc làm này:
- *Sega Enterprises v. Accolade* (9th Cir. 1992): reverse engineering để đạt được interoperability là fair use.
- *Sony Computer Entertainment v. Connectix* (9th Cir. 2000): tương tự.
- *Google LLC v. Oracle America* (SCOTUS 2021): khẳng định fair use trong interoperability context.
- EU Software Directive Article 6 (Directive 2009/24/EC): cho phép decompile/reverse để interoperability, bao gồm cả khi cần tạo phần mềm tương thích đọc cùng file format.
- App tương tự đã làm công khai: JJazzLab (open-source, parse .sty), vArranger (thương mại, Windows), Band-in-a-Box (parse nhiều format). Không có tiền lệ bị kiện.

**Điều kiện để rủi ro ở mức thấp:**
1. Chỉ parse file mà user import — không download, không phân phối .sty.
2. Không vi phạm DMCA §1201: .sty không bị mã hóa/DRM ở tầng format (chỉ là binary data format). DRM chỉ xuất hiện ở Expansion Pack (.ppi/.cpi) — hoàn toàn khác.
3. Không copy implementation code từ Yamaha (chỉ học format/behavior).

**Chính sách:**
- Cho phép. Ghi rõ trong ToS: "User chịu trách nhiệm đảm bảo họ có quyền hợp pháp với file import."
- Không cung cấp công cụ download .sty từ internet.
- Không bundle .sty file không rõ nguồn gốc.

---

### L2 — Bypass DRM / Mã hóa Expansion Pack

**Kết luận: CẤM TUYỆT ĐỐI. Không thảo luận thêm.**

DMCA §1201(a): "No person shall circumvent a technological measure that effectively controls access to a work protected under this title."

Yamaha Expansion Pack (.ppi, .cpi) được mã hóa và ràng buộc với hardware ID của instrument cụ thể. Bất kỳ hành động nào nhằm decrypt hay bypass đều là vi phạm DMCA §1201, có thể bị truy tố hình sự (§1204: lên đến $1M fine + 10 năm tù tái phạm).

**Điều không được làm:**
- Không implement decryption.
- Không tài liệu hóa cách decrypt, kể cả dạng "research".
- Không nhận code từ bên thứ ba thực hiện decryption.
- Không cung cấp tool trích xuất sample từ Expansion Pack.

**Thay thế hợp pháp:** user sample import (WAV/SFZ/SF2) + AI-assisted remap (phase 2) là con đường đúng.

---

### L3 — ROM Sample của Yamaha/Korg/Roland/Ketron

**Kết luận: CẤM TUYỆT ĐỐI.**

ROM sample là tác phẩm sáng tạo được bảo hộ bản quyền. Bundle, phân phối, hay hướng dẫn extract đều là copyright infringement trực tiếp.

**Không được làm:**
- Bundle bất kỳ sample nào từ Yamaha/Korg/Roland.
- Hướng dẫn user extract ROM sample từ instrument.
- Tạo "conversion tool" ngầm định mục đích extract ROM.

**App store policy:** Apple sẽ reject app có bundled copyrighted audio không được license.

---

### L4 — Trademark Risk (Quan trọng nhất cần chú ý)

**Mức rủi ro: Vừa — đây là rủi ro thực tế nhất cần quản lý chủ động.**

**Các tên thương hiệu cần tránh vi phạm:**
- Yamaha® — YAMAHA CORPORATION
- Genos® — Yamaha
- Tyros® — Yamaha
- Korg® — KORG Inc.
- Pa5X® — Korg
- Roland® — Roland Corporation
- Ketron® — Ketron S.r.l.
- GarageBand® — Apple
- Logic Pro® — Apple
- MainStage® — Apple

**Quy tắc nominative fair use (được phép về mặt trademark):**
```
✅ "Imports styles in Yamaha® .STY format"
✅ "Supports Yamaha SFF1/SFF2 format"
✅ Disclosure: "Yamaha is a registered trademark of Yamaha Corporation.
   This app is not affiliated with or endorsed by Yamaha Corporation."
```

> ⚠️ **Claims policy override:** the bare phrase "Compatible with Yamaha PSR/Tyros
> style files" — while permissible under trademark nominative fair use — is
> **NOT allowed** until real-device validation evidence is committed, per
> `docs/policy/CLAIMS_AND_RELEASE_NOTES_POLICY.md`
> (HARDWARE_REVIEW_STATUS: DEFERRED_BY_PTH). Use instead:
> "Engine-side Yamaha SFF/CASM importer (hardware parity unverified)".

**Tuyệt đối không được:**
```
❌ Tên app: "Yamaha Arranger", "Genos App", "KorgPlayer"
❌ Icon/screenshot: logo hoặc trade dress của Yamaha/Korg
❌ "Replaces your Yamaha Genos" (comparative advertising có thể gây vấn đề)
❌ "Official Yamaha style player" hoặc bất kỳ gợi ý official endorsement
❌ Domain: yamaha-arranger.com, genos-app.com, v.v.
```

**Action items trước launch:**
1. Trademark search tên app: USPTO (US) + EUIPO (EU) + JPO (Japan — Yamaha home country). Chi phí: ~$500 tự tìm, ~$2,000 qua luật sư.
2. Không đặt tên app có từ Yamaha/Korg/Roland/Genos/Pa5X.
3. App Store description: có disclosure "not affiliated with" rõ ràng.
4. Marketing copy review: mọi so sánh với hardware arranger phải factual, không misleading.

---

### L5 — JUCE License

**Mức rủi ro: Thấp nếu mua đúng tier.**

JUCE license tiers (tại thời điểm viết tài liệu — kiểm tra juce.com vì có thể thay đổi):
- **Personal/Indie:** free nếu doanh thu < $50k/năm; có phí nhỏ cho $50k–$500k.
- **Pro:** cho doanh thu cao hơn.

**Quy tắc:**
- Mua JUCE Indie license từ đầu — không dùng free tier rồi "nâng sau" (có thể vi phạm ToS trong thời gian đó).
- JUCE Indie cho phép App Store distribution. Không cần dynamic linking (issue cũ với LGPL đã không còn liên quan với JUCE 7 commercial license).
- Khi doanh thu vượt ngưỡng → upgrade license trước khi vượt, không sau.

---

### L6 — Open-Source License Compliance

**Mức rủi ro: Vừa — cần audit định kỳ.**

**Phân loại license các dependency:**

| Thư viện | License | Yêu cầu |
|---|---|---|
| sfizz | BSD-2-Clause | Giữ copyright notice trong About screen / NOTICE file. Không cần source disclosure. |
| Airwindows | MIT | Giữ copyright notice. |
| GRDB.swift | MIT | Giữ copyright notice. |
| GoogleTest | BSD-3-Clause | Chỉ dùng trong test target, không ship production. |
| zita-rev1 (nếu dùng) | GPL-3 | **KHÔNG dùng trong production app** — GPL contaminate. Thay bằng Airwindows reverb (MIT) hoặc JUCE reverb. |
| GeneralUser GS soundfont | Custom free license | Cho phép phân phối trong app, attribution required. Kiểm tra lại license hiện tại trước bundle. |

**Action items:**
- Tạo `THIRD_PARTY_LICENSES.txt` trong app bundle (accessible từ Settings/About).
- Audit mọi dependency mới trước khi add vào project.
- Rule: không add dependency GPL/AGPL vào production target, bất kể lý do.
- Viết OSS License Audit script chạy trong CI để detect license thay đổi.

---

### L7 — Soundfont / SFZ Bundle Licensing

**Mức rủi ro: Thấp nếu chọn đúng source.**

**Lựa chọn bundled GM soundfont (theo thứ tự ưu tiên):**

| Soundfont | License | Ghi chú |
|---|---|---|
| GeneralUser GS | Custom free, allow distribution | Attribution required. Đã dùng rộng rãi trong apps. Kiểm tra license page. |
| MuseScore GM soundfont | GPL-2 | **Không dùng** — GPL. |
| Fluid R3 GM | MIT/LGPL dual | MIT phần có thể dùng. Kiểm tra specific files. |
| SGM-V2.01 | CC-BY | Attribution trong About screen. |
| Yamaha XG soundfont (bất kỳ) | Proprietary | **Không dùng.** |

**Quyết định:** dùng GeneralUser GS (đã proven trong apps) + verify license hiện tại một lần nữa trước bundle.

**User-imported soundfont:** user chịu trách nhiệm. ToS phải nói rõ.

---

### L8 — User Import Content Liability

**Mức rủi ro: Vừa — mitigate bằng ToS + design.**

Khi user import .sty file, app không thể kiểm tra xem user có hợp pháp sở hữu file đó không. Rủi ro: user import style đã download lậu từ internet.

**Giảm thiểu:**
1. **ToS condition:** "You represent that you have the legal right to use any files you import." Explicit warranty by user.
2. **Design:** app không có built-in browser/downloader. User phải dùng Files app để import — rõ ràng là user action, không phải app provide.
3. **No facilitation:** app không link đến sites phân phối style file, không có "style store" ngoài official marketplace của ta (phase 3).
4. **Safe harbor:** nếu đủ lớn để relevant, apply DMCA safe harbor (17 U.S.C. §512(c)) — cần DMCA agent đăng ký với Copyright Office (~$6/3 năm).

---

### L9 — Marketplace Liability (Phase 3 — Chuẩn bị từ bây giờ)

Khi marketplace mở, platform có thể chịu secondary liability nếu creator upload nội dung vi phạm.

**Infrastructure cần chuẩn bị trước khi mở marketplace:**
1. **DMCA Agent đăng ký** với US Copyright Office.
2. **Takedown process** rõ ràng (receive → xem xét 24h → remove nếu hợp lý → counter-notice process).
3. **Creator agreement** bao gồm: warranty of ownership, indemnification clause, right to remove.
4. **Pack metadata** bắt buộc: `sampleOwnership` field (đã có trong UASF schema). Creator phải declare.
5. **Review process** trước khi approve pack vào marketplace (manual + automated scan cho known copyrighted audio — AudD API hoặc tương đương).
6. **Stripe Atlas / LLC** hoặc entity phù hợp trước khi nhận payment từ creators.

---

### L10 — Patent Risk (DSP Algorithms)

**Mức rủi ro: Thấp.**

Các DSP algorithm ta dùng (reverb, compressor, EQ, chorus) đều dựa trên kỹ thuật đã published từ thập niên 1980–90s, hầu hết patent đã expired. Arranger logic (NTT/NTR) là musical theory, không patentable.

Rủi ro nhỏ còn lại: một số patent submarine về audio processing. Mitigation: dùng implementation từ thư viện đã established (JUCE, Airwindows) — nếu có patent issue, đó là vấn đề của thư viện đó, không phải ta (indemnification thường có trong commercial license).

---

## 4. TERMS OF SERVICE — DRAFT CÁC ĐIỀU KHOẢN QUAN TRỌNG

*(Draft nội bộ — cần luật sư review trước publish)*

**§ File Import:**
"You may only import files that you own or have obtained the legal right to use. By importing a file, you represent and warrant that you have all necessary rights to that file. [App Name] does not verify ownership and is not responsible for files imported by users."

**§ Sound Samples:**
"[App Name] does not include and does not assist in obtaining manufacturer ROM samples, expansion pack content, or any other proprietary audio assets from instrument manufacturers. Users are solely responsible for the legality of any audio content they use within the application."

**§ Manufacturer Trademarks:**
"Yamaha, Genos, Tyros, Korg, Pa5X, Roland, and Ketron are trademarks of their respective owners. [App Name] is not affiliated with, endorsed by, or sponsored by any of these companies."

**§ User Content (Marketplace — phase 3):**
"By uploading content to [App Name] Marketplace, you represent that: (a) you own or have licensed all content; (b) the content does not infringe any third-party rights; (c) you grant [App Name] a license to distribute your content. We reserve the right to remove content at our discretion."

**§ DMCA:**
"We respect intellectual property rights. To report infringement, contact [dmca@appname.com]. We will process notices in accordance with the DMCA."

---

## 5. SAMPLE LICENSING POLICY (Nội bộ + Marketplace)

**Tier A — Hoàn toàn tự do:**
- User's own recordings.
- CC0 / Public Domain samples.
- Samples user đã mua license cho redistribution.

**Tier B — Dùng nội bộ, không redistribute:**
- Samples từ library user đã mua (Splice, Loopmasters, v.v.) — thường có license "use in production, no redistribution as standalone sample".
- User phải tự verify license của mình. App không enforce.

**Tier C — Cấm hoàn toàn:**
- Yamaha/Korg/Roland ROM samples.
- Samples extract từ commercial VI (Native Instruments, Spectrasonics, v.v.).
- Samples không rõ nguồn gốc.

**Marketplace enforcement (phase 3):**
- Creator declare tier khi upload.
- Automated fingerprinting (AudD hoặc tương đương) cho known commercial samples.
- Human review trước approve.
- Violation → immediate takedown + creator account review.

---

## 6. ACTION ITEMS THEO MILESTONE

| Milestone | Action | Owner | Deadline |
|---|---|---|---|
| Sprint 0 | Mua JUCE Indie license | CTO/Founder | Tuần 1 |
| Sprint 0 | Chốn tên app, bắt đầu trademark search | Founder | Tuần 2 |
| Sprint 0 | Audit OSS licenses, viết NOTICE file | iOS Eng | Tuần 4 |
| Tháng 3 | Verify GeneralUser GS license hiện tại | iOS Eng | Trước bundle soundfont |
| Tháng 5 | ToS + Privacy Policy draft hoàn chỉnh | Founder + luật sư | 4 tuần trước TestFlight |
| Tháng 6 (pre-launch) | Luật sư review: trademark + ToS | Luật sư IP | 2 tuần trước submit |
| Tháng 6 (pre-launch) | App Store description review (trademark usage) | Founder | 1 tuần trước submit |
| Phase 3 (marketplace) | DMCA agent đăng ký | Luật sư | 1 tháng trước marketplace open |
| Phase 3 | Creator agreement + payout entity setup | Luật sư + accountant | 2 tháng trước marketplace |

---

## 7. IP CHÚNG TA TẠO RA — BẢO VỆ

| Tài sản IP | Loại | Bảo vệ |
|---|---|---|
| UASF format specification | Trade secret + Copyright (tài liệu) | Không publish spec; code là copyright tự động |
| Arranger Engine (NTT transform implementation) | Copyright + Trade secret | Không open-source; closed source |
| Universal Internal Style Format | Copyright (tài liệu + code) | Tương tự |
| App UX design | Copyright + Trade dress | Không cần patent; đăng ký design patent nếu distinctive enough |
| Tên app + logo | Trademark | Đăng ký USPTO ngay khi chọn tên |
| Marketplace creator tools (phase 3) | Copyright | Tương tự |

**Trade secret protection:** NDA cho mọi contractor/employee; không post algorithm details lên StackOverflow/GitHub public; internal docs không chia sẻ ra ngoài.
