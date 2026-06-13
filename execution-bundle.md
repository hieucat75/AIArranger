# EXECUTION DOCUMENTS BUNDLE
**Team Requirements + Build-vs-Buy Matrix + Risk Register + Open Questions**
**Live Arranger Platform — v0.9**

---

# DOCUMENT A: TEAM & SKILL REQUIREMENTS

## A.1. Cơ cấu team MVP

### Role 1 — Senior C++ Realtime Audio Engineer (FULL-TIME, BẮT BUỘC)

**Đây là vị trí quan trọng nhất. Không tuyển được người phù hợp = không nên bắt đầu.**

Yêu cầu kỹ thuật:
- ≥ 5 năm kinh nghiệm C++ audio/DSP production code.
- Đã từng implement hoặc maintain realtime audio engine (game audio, DAW plugin, embedded synth, hay tương đương).
- Hiểu sâu: lock-free data structures, memory model C++11+, audio callback constraints.
- Biết CoreAudio hoặc ASIO (iOS/macOS preferred, Windows acceptable).
- Biết MIDI protocol, JUCE là plus lớn.
- Có thể đọc và hiểu SMF format, MIDI spec từ byte level.

Red flags (không phù hợp):
- Chỉ viết audio code "hobby" hoặc academic — không biết production constraints.
- Không biết tại sao không được malloc trong audio thread.
- Chưa từng debug timing issue bằng oscilloscope hoặc hardware MIDI analyzer.

Compensation range (Vietnam market): $3,000–5,000/tháng gross. Nếu remote quốc tế: $6,000–10,000/tháng.

**Interview challenge:** "Viết minimal SPSC lock-free ring buffer trong C++ và giải thích memory ordering." + "Tại sao không dùng std::mutex trong audio callback và bạn sẽ làm gì thay thế?"

---

### Role 2 — Senior iOS/SwiftUI Engineer (FULL-TIME)

Yêu cầu:
- ≥ 3 năm Swift/SwiftUI production.
- Hiểu AVAudioSession lifecycle, background audio, interruption handling.
- Biết CoreMIDI (basic) — đủ để viết device enumeration và hot-plug.
- Kinh nghiệm Swift/C++ interop (hoặc Objective-C bridge — sẵn sàng học interop mới).
- Biết SQLite/GRDB hoặc CoreData.
- Có sense về touch UX cho music app là plus.

Không cần phải biết audio DSP — đó là Role 1.

Compensation: $2,500–4,000/tháng (Vietnam). Remote: $5,000–8,000/tháng.

---

### Role 3 — Domain Expert / Nhạc Công Arranger (PART-TIME hoặc CO-FOUNDER)

**Không phải engineer — nhưng thiếu vai trò này thì sản phẩm sẽ sai về âm nhạc.**

Cần người:
- Chơi keyboard arranger chuyên nghiệp (Yamaha PSR/Genos/Korg Pa).
- Biết sâu về style: NTT behavior, retrigger "feel", chord voicing.
- Có thể A/B test engine output với Yamaha reference và nói "bass line này sai vì...".
- Có network trong cộng đồng nhạc công Việt Nam/quốc tế để recruit beta users.

Không cần: coding skills.

Compensation: equity stake (co-founder) hoặc $500–1,000/tháng consultant.

---

### Role 4 — Designer (PART-TIME, tháng 4–5)

Yêu cầu:
- Portfolio có iOS music/creative app.
- Hiểu live performance UX constraints (dark mode, large targets, glanceability).
- Figma proficient.
- Không cần biết code, nhưng biết iOS HIG là plus.

Không cần: brand identity (chỉ cần product design cho MVP).

Scope: Live Screen refinement, Mixer UI, Style Browser, onboarding.

Compensation: $2,000–4,000 project-based (tháng 4–5).

---

### Role 5 — QA / Beta Coordinator (PART-TIME, từ tháng 4)

Yêu cầu:
- Biết TestFlight, Crashlytics.
- Kinh nghiệm test musical instrument apps hoặc MIDI apps là lớn plus.
- Có thể điều hành beta community (feedback form, video calls).
- Biết cơ bản về MIDI và arranger (để hiểu bug report từ nhạc công).

Compensation: $800–1,500/tháng part-time.

---

## A.2. Hiring Timeline

```
Tháng 1 (Sprint 0):  C++ Engineer + iOS Engineer bắt đầu ngay
Tháng 1:             Xác nhận Domain Expert / nhạc công cố vấn
Tháng 4:             Designer onboard (sprint 3)
Tháng 4:             QA onboard
```

## A.3. Knowledge Transfer & Documentation

Mọi quyết định kỹ thuật → ADR document. Mọi thuật toán phức tạp → inline comment + spec reference. Không để knowledge chỉ nằm trong đầu 1 người.

Đặc biệt: NTT transform logic và CASM parser phải có unit test coverage 100% và spec document rõ ràng — đây là risk nếu C++ engineer nghỉ việc.

---

# DOCUMENT B: BUILD-vs-BUY DECISION MATRIX (CHI TIẾT)

## B.1. Framework đánh giá

Mỗi component được đánh giá theo 5 tiêu chí (1–5):
- **Effort to build:** thời gian nếu tự làm (1=ít, 5=nhiều)
- **Risk if built:** rủi ro nếu tự làm sai (1=thấp, 5=cao)
- **Quality of available:** chất lượng giải pháp sẵn có (1=kém, 5=tốt)
- **License compatibility:** license cho iOS App Store (1=không ổn, 5=hoàn toàn ổn)
- **Long-term flexibility:** khả năng thay thế sau này (1=khó, 5=dễ)

Score BUY = Quality + License + (-Risk if built)
Score BUILD = Flexibility + (-Effort) + Core IP value

## B.2. Ma trận

| Component | Decision | Solution | Build Effort | Notes |
|---|---|---|---|---|
| **Arranger Engine** | BUILD | C++ internal | 8 weeks | Core IP — không thể mua |
| **Chord Engine** | BUILD | C++ internal | 2 weeks | Core IP, nhỏ, dễ test |
| **Style Parser (Yamaha)** | BUILD | C++ internal | 4 weeks | Core IP — không có library tốt |
| **UASF Format** | BUILD | Internal | 1 week | Core IP |
| **Audio/MIDI backbone** | BUY | JUCE 7 Indie | 0 weeks | Tiết kiệm 3–6 tháng, license OK |
| **Sampler** | BUY | sfizz (BSD-2) | 1 week integration | Không viết sampler từ zero |
| **Reverb (phase 1)** | BUY | JUCE DSP Reverb | 0.5 weeks | Quality "OK" — upgrade phase 2 |
| **Reverb (phase 2)** | BUY | Airwindows MVerb (MIT) | 1 week port | Chất lượng cao hơn |
| **Compressor/Limiter** | BUY | JUCE DSP | 0.5 weeks | Đủ tốt |
| **EQ (phase 2)** | BUY | JUCE DSP | 0.5 weeks | 7-band |
| **MIDI I/O** | HYBRID | CoreMIDI (native) + JUCE wrapper tắt | 1 week | CoreMIDI tốt hơn JUCE MIDI trên iOS |
| **Swift/C++ bridge** | BUILD | Swift/C++ Interop (Xcode 15+) | 2 weeks | Chính thức Apple |
| **Database** | BUY | SQLite + GRDB | 0.5 weeks | Proven, MIT |
| **Crash reporting** | BUY | Crashlytics (free tier) | 0.5 weeks | Standard |
| **Time-stretch** | DEFER | Rubber Band / Signalsmith | — | Phase 3 (Ketron audio style) |
| **AI remap** | DEFER | ONNX / CoreML | — | Phase 2+ |
| **Cloud backend** | DEFER | Firebase / AWS | — | Phase 3 (marketplace) |
| **DRM** | DEFER | Custom | — | Phase 3 |

## B.3. Quyết định đặc biệt: "Không mua AUv3 host framework"

Có thể dùng AudioKit hoặc Audiobus SDK để hosting AUv3 nhanh hơn. Lý do KHÔNG làm trong MVP:
- AUv3 hosting thêm 1–2 tháng chỉ để làm đúng (sandbox, state management, undo).
- MVP không cần — external MIDI out đủ cho nhạc công.
- Phase 2 thêm tự nhiên khi engine stable.

## B.4. Quyết định đặc biệt: "Không dùng AudioKit cho core"

AudioKit là Swift framework tốt nhưng:
- C++ core cần portable (macOS native, tương lai Windows) — AudioKit Swift-only.
- Performance critical code cần control tuyệt đối — AudioKit là abstraction layer không cần.
- JUCE đã cover phần lớn functionality với license clear hơn.

AudioKit có thể useful cho UI-level audio utilities (waveform display, v.v.) ở phase 2.

---

# DOCUMENT C: RISK REGISTER

| ID | Rủi ro | Category | Probability | Impact | Score | Mitigation | Owner | Status |
|---|---|---|---|---|---|---|---|---|
| R01 | Không tuyển được C++ audio engineer đủ giỏi | Team | High | Critical | 🔴 HIGH | Bắt đầu tuyển song song với spec; network audio engineering community; consider remote/quốc tế | Founder | OPEN |
| R02 | SFF1 CASM bit-level khác spec tổng hợp | Technical | Med | High | 🟡 MED | Sprint 0 spike xác minh với corpus; JJazzLab cross-ref; không code blind | C++ Eng | OPEN |
| R03 | Timing jitter trên iPad thật cao hơn dự kiến | Technical | Med | High | 🟡 MED | Spike đo sớm; fallback tăng buffer; CI timing stress test | C++ Eng | OPEN |
| R04 | sfizz không đủ tốt cho multisample phase 2 | Technical | Low | Med | 🟢 LOW | sfizz đang active dev; có thể swap sampler vì interface abstract | C++ Eng | WATCH |
| R05 | JUCE license thay đổi hoặc tăng giá | Legal/Tech | Low | Med | 🟢 LOW | Architecture cho phép swap audio backbone (Bridge layer mỏng); theo dõi JUCE blog | CTO | WATCH |
| R06 | Apple reject App Store vì MIDI permission hoặc format claim | Legal | Low | High | 🟡 MED | Review App Store Guidelines sớm; test với TestFlight review trước submit | iOS Eng | OPEN |
| R07 | Yamaha gửi C&D về trademark usage trong marketing | Legal | Low | High | 🟡 MED | Chỉ nominative fair use; luật sư review description; không dùng logo/trade dress | Founder | OPEN |
| R08 | Beta nhạc công crash trong show thật | Product | Med | Critical | 🔴 HIGH | 2h stability test × 3 pass trước khi invite beta; Crashlytics realtime; hotfix trong 24h | All | OPEN |
| R09 | SFF2 import quality không đủ tốt → user complain | Product | Med | Med | 🟡 MED | Set expectation rõ (80% compatibility claim); limitation UI rõ ràng; degrade gracefully | Team | OPEN |
| R10 | Competitive: vArranger ra iOS version | Market | Low | High | 🟡 MED | Tăng tốc launch; differentiate bằng UX + open platform; community first-mover | Founder | WATCH |
| R11 | User không chấp nhận trả tiền (WTP thấp hơn dự kiến) | Business | Med | Med | 🟡 MED | Freemium model: thử trước; pricing test với beta; one-time thay subscription | Founder | OPEN |
| R12 | iOS update breaking CoreMIDI / AVAudioSession | Technical | Low | High | 🟡 MED | Test trên iOS beta mỗi năm; abstract platform layer trong architecture | iOS Eng | WATCH |
| R13 | Domain expert (nhạc công) không có đủ thời gian | Team | Med | Med | 🟡 MED | Define minimum commitment (4h/tuần review + beta coordination); equity incentive | Founder | OPEN |
| R14 | Test corpus style có style lậu (copyright issue) | Legal | Low | Low | 🟢 LOW | Beta team chỉ export từ máy họ sở hữu; không lưu corpus trên shared cloud | QA | OPEN |
| R15 | Memory leak trong 2h session (C++ ARC mismatch) | Technical | Med | Med | 🟡 MED | ASan trong debug build; memory test nightly; RAII discipline nghiêm ngặt | C++ Eng | OPEN |

### Risk Review Cadence
- Weekly: review open HIGH risks
- Sprint end: review tất cả risks, update status
- Milestone: full risk register review, close resolved, add new

---

# DOCUMENT D: OPEN QUESTIONS LIST

*Tổng hợp từ tất cả spec documents — chốt theo thứ tự ưu tiên*

## D.1. Sprint 0 (phải chốt trước khi code engine)

| # | Question | Document gốc | Chốt bằng cách nào |
|---|---|---|---|
| OQ-01 | Ctab field offsets chính xác (byte level): srcChannel, dstChannel, NTR, NTT, RTR vị trí chính xác? | File Import Strategy | Hex dump 10 style + cross-ref JJazzLab source |
| OQ-02 | chordMuteBitmask SFF1: thứ tự bit khớp 34 chord type enum nào? | File Import Strategy | Thử nghiệm: set mute, phát style, kiểm tra track nào im |
| OQ-03 | Sdec encoding: ASCII thuần hay có thể Shift-JIS? | File Import Strategy | Test style tiếng Nhật download từ forum |
| OQ-04 | CASM position: luôn sau MTrk hay có file nhúng trong track riêng? | File Import Strategy | Scan toàn bộ corpus, kiểm tra structure |
| OQ-05 | SFF2 numRanges: vị trí chính xác trong Ctb2, trước hay sau field cơ bản? | File Import Strategy | Cross-ref JJazzLab YamJJazz Ctb2 parser |
| OQ-06 | Bảng NTT (đặc biệt mode "melody"): thuật toán baseline có "nghe đúng" không? | Arranger Engine Spec | A/B test với Yamaha reference (nhạc công chấm điểm) |

## D.2. Sprint 1 (chốt trước khi freeze engine API)

| # | Question | Document gốc | Chốt bằng cách nào |
|---|---|---|---|
| OQ-07 | Virtual MIDI port: 1 port mix all tracks hay 16 port per-track? | MIDI Engine | Beta musician survey (10 người) |
| OQ-08 | Manual bass mode: có thực sự cần trong MVP? Có style/workflow nào yêu cầu? | Chord Engine | Hỏi domain expert + beta cohort |
| OQ-09 | Chord change ngay giữa fill: áp transform lên fill hay khóa chord đến hết fill? | Arranger Engine | A/B test, hỏi nhạc công |
| OQ-10 | Sync Stop vs Instant Stop: mặc định nào cho nhạc công VN? | Arranger Engine | Domain expert + beta survey |
| OQ-11 | noteGenerator retrigger: style phổ biến nào dùng? Nếu hiếm, giữ fallback lâu dài? | Arranger Engine | Corpus analysis: scan RTR=noteGenerator, đếm |
| OQ-12 | Style 3/4 và 6/8: có cần `timeSignature` per-section trong UASF không? | UASF Spec | Corpus analysis: đếm style đổi timesig giữa section |

## D.3. Sprint 2–3 (design decisions)

| # | Question | Document gốc | Chốt bằng cách nào |
|---|---|---|---|
| OQ-13 | On-screen chord pad: giữ root + tap type, hay 1 nút = tổ hợp? | Chord Engine | Usability test với 5 nhạc công |
| OQ-14 | Chord latch threshold: 80ms có phù hợp với playing style thực tế? | Chord Engine | A/B test với nhạc công chơi fast chord |
| OQ-15 | Có giữ source.bin trong .uas mặc định bật không? (dung lượng vs re-import) | UASF Spec | Đo kích thước corpus, survey nhạc công về storage concern |
| OQ-16 | Single Finger mode edge case: phím đen đơn lẻ khi bassZone tắt? | Chord Engine | Test trên PSR thật, map exact behavior |
| OQ-17 | Aftertouch/pitch bend tay trái: bỏ qua hay forward về MIDI Engine? | Chord Engine | Domain expert + beta feedback |
| OQ-18 | Pricing final: $29.99 vs $39.99 one-time? Có subscription tier không? | BRD | Pricing survey beta users + comparable apps research |

## D.4. Pre-launch (legal/business)

| # | Question | Document gốc | Chốt bằng cách nào |
|---|---|---|---|
| OQ-19 | Tên app final: trademark clear ở US/EU/JP? | Legal | Luật sư IP trademark search |
| OQ-20 | GeneralUser GS license hiện tại: vẫn cho phép bundle trong commercial app không? | Legal | Email tác giả + đọc license file mới nhất |
| OQ-21 | MIDI clock in: cần ở MVP không? Có nhạc công nào cần sync với drum machine? | MIDI Engine | Beta survey |
| OQ-22 | AUv3 hosting: có đủ demand từ beta để accelerate vào MVP không? | PRD | Beta feedback round 1 |

---

## D.5. Tracking & Resolution Process

Mỗi OQ phải được resolve và close trước khi component liên quan được code.

```
OQ lifecycle:
  OPEN → INVESTIGATING → DECIDED → CLOSED

Với mỗi closed OQ, lưu:
  - Decision made
  - Rationale
  - Date
  - Who decided
  - Affected documents (update spec)
```

Ví dụ closed OQ:
```
OQ-07: Virtual MIDI port count
Status: CLOSED (Sprint 0, Week 4)
Decision: 1 port (all tracks mixed)
Rationale: Beta survey 8/10 người chỉ cần 1 port; multi-port adds complexity không có demand rõ ràng
Affected: MIDI Engine Spec §3.3 — đã update
```
