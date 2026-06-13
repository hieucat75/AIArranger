# TESTING STRATEGY — LIVE ARRANGER PLATFORM
**Trạng thái: Draft — Áp dụng từ Sprint 0**

---

## 1. TRIẾT LÝ KIỂM THỬ

**Test không phải để "chứng minh không có lỗi" — test để bắt được lỗi trước nhạc công.**

Ba yêu cầu phi chức năng không thể negotiate:
1. **Không crash trong 2 giờ live session.**
2. **Section switch không lệch nhịp — bao giờ.**
3. **Chord recognition không để note kẹt — bao giờ.**

Mọi test case, CI gate, và beta protocol đều phục vụ ba yêu cầu này.

---

## 2. TẦNG KIỂM THỬ

```
[Unit Tests]          ← nhanh, chạy mọi commit, developer tự chạy
      ↓
[Integration Tests]   ← chạy mọi commit trong CI, ~5 phút
      ↓
[System Tests]        ← chạy nightly, ~30 phút
      ↓
[Performance Tests]   ← chạy nightly, trên thiết bị thật
      ↓
[Beta / Field Tests]  ← nhạc công thật, show thật
```

---

## 3. UNIT TESTS

### 3.1. Chord Engine (Module 5)

**Mục tiêu coverage:** 100% recognition cases trong lookup table.

```
Test suite: ChordRecognitionTests
  [Exact match] 34 types × 12 roots × root position          = 408 cases
  [Inversions]  14 MVP types × 12 roots × 2 inversions       = 336 cases
  [Partial]     thiếu 1 note, dư 1 note — 50 cases manual
  [Edge]        1 note, 2 note, >5 note, empty                = 20 cases
  [SingleFinger] mọi tổ hợp quy ước Yamaha                   = 30 cases
  [Slash chord]  bass ≠ root, nhiều octave                    = 24 cases
  [Hold mode]    release, partial release, chord latch 80ms   = 15 cases
  Total target: ~900 cases, tất cả automated

Test format:
  input:  Set<uint8_t> heldNotes
  expect: Chord {root, type, bassNote}
  assert: exact match
```

**Property tests (fuzz):**
- Random `heldNotes` bitmask → recognize() không crash, trả về valid Chord.
- `root ∈ [0,11]`, `bassNote ∈ [0,127]`, `type ∈ ChordTypeEnum`.
- Chạy 100,000 random inputs mỗi CI run.

### 3.2. Arranger Engine — Chord Transform (Module 4)

```
Test suite: NTRTransformTests
  rootTransposition × 12 source root × 12 target root × 10 note positions = 1,440 cases
  rootFixed         × tương tự                                             = 1,440 cases
  noteLimit wrap    (below min, above max, multiple octaves)                = 60 cases

Test suite: NTTTransformTests
  6 NTT modes × 34 chord types × 12 note input positions                  = 2,448 cases
  Bất biến: output pitch-class phải thuộc target chord/scale              = assert

Test suite: RetriggerTests
  6 policies × note đang vang × chord change → verify NoteOff/NoteOn sequence
  Bất biến: sau retrigger, activeNotes count không đổi (hoặc đổi đúng policy)
```

**Critical invariants (assert trong mọi transform test):**
```
∀ note pipeline:
  0 ≤ output ≤ 127                    // không bao giờ ngoài MIDI range
  noteLowLimit ≤ output ≤ noteHighLimit  // limit wrap đúng
  chordMuted note → không emit         // mute hoạt động
  pitchClass(output) ∈ targetScale     // NTT đúng mode
```

### 3.3. Arranger Engine — State Machine (Module 4)

```
Test suite: SectionStateMachineTests
  Start → PLAYING, first section = Intro nếu có / Main A nếu không
  Pending → commit tại đúng boundary (fill: beat, main/ending: bar)
  Loop: Main A lặp đúng, playCursor wrap tại lengthBeats
  Auto fill: Main A→B với autoFill=on → Fill AA chèn trước Main B
  Ending: chạy hết → STOPPED (không loop)
  Panic: STOPPED bất kỳ lúc nào, activeNotes = 0 sau đó
  Section switch mid-bar: pending đặt, commit đúng bar kế
  Double ending: nhấn Ending 2 lần → Ending B (nếu có) thay Ending A
```

### 3.4. UASF Import / Validation

```
Test suite: UASFValidationTests
  V01–V07: mỗi lỗi một file fixture → expect ImportResult.success = false
  W01–W05: mỗi warning → expect ImportResult.warnings chứa đúng code
  Round-trip: parse UASF → serialize → parse lại → identical
  Migration: file v0.9 → engine v1.0 → migrate thành công
```

### 3.5. SFF1 Parser

```
Test suite: SFF1ParserTests
  Fixtures: 20 file .sty thật từ corpus (đa dạng model)
  Assert: section count đúng, CASM parse được, soundMap có giá trị hợp lệ
  Limitation codes: file có cross-fill → FILL_CROSS_DEGRADED; v.v.
  Unknown chunk → skip gracefully, không crash
  Corrupted file (cắt giữa chừng) → ImportResult.success=false, không crash
```

---

## 4. INTEGRATION TESTS

### 4.1. Full pipeline smoke test

```
Test: "SFF1 → UASF → Arranger Engine → MIDI output"
  1. Import 5 style SFF1 khác nhau → verify UASF hợp lệ
  2. Load UASF vào ArrangerEngine
  3. PostCommand: Start
  4. PostChord: CMaj, Am, F, G (4 bar)
  5. PostCommand: Fill
  6. PostCommand: SetSection(main_b)
  7. PostCommand: Ending
  8. Collect output MIDI events
  9. Assert:
     - Không crash
     - Mọi Note On có matching Note Off
     - Section switch xảy ra tại đúng boundary (±2 tick tolerance)
     - Chord change → retrigger events ở đúng tick
     - Ending → STOPPED sau khi ending hết
```

### 4.2. Panic integration test

```
  1. Start arranger, chạy 100 tick
  2. Inject 20 Note On events vào activeNotes thủ công
  3. PostCommand: Panic
  4. Assert:
     - activeNotes.size() == 0
     - Output queue chứa đủ Note Off cho mọi channel 0–15
     - Transport: STOPPED
     - Tiếp theo PostCommand: Start → hoạt động bình thường
```

### 4.3. MIDI device disconnect integration

```
  1. Mock MIDI device kết nối, Start arranger
  2. Simulate device disconnect
  3. Assert:
     - Transport tiếp tục (không dừng)
     - Sound Engine fallback bật (nếu hybrid mode)
     - UI state: cảnh báo disconnect
  4. Simulate reconnect
  5. Assert: init sequence được gửi, output resume
```

---

## 5. SYSTEM TESTS (nightly, thiết bị thật)

### 5.1. Timing stress test — quan trọng nhất

```
Duration: 30 phút render liên tục
Style: 5 style khác nhau, xoay vòng
Chord changes: random mỗi 2–8 beat
Section switches: random mỗi 4–16 bar
CPU load injection: background task (import 50 style) trong khi chạy

Metric đo:
  Event timing error = |actualTick - scheduledTick|
  Assert: P50 < 0.5 tick, P99 < 2 tick, max < 5 tick
  (tại 480 PPQ 120 BPM: 2 tick ≈ 2ms — ngưỡng nghe được)

Orphan notes: scan toàn bộ output, đếm NoteOn không có matching NoteOff
  Assert: 0 orphan notes sau 30 phút
```

### 5.2. Memory stability test

```
Duration: 2 giờ (đúng requirement của brief)
Actions: import 20 style, chơi live, switch style 50 lần, panic 10 lần

Metric:
  Memory usage: không tăng liên tục (leak detection)
  CPU usage: không tăng liên tục
  Crash: 0
  Assert: memory sau 2h ≤ memory sau 10 phút × 1.1
```

### 5.3. Import corpus test

```
Input: toàn bộ corpus 500+ file
Assert per file:
  - Không crash parser
  - success hoặc có error code rõ ràng (không silent fail)
  - Nếu success: UASF valid (V01–V07)
  - Section count ≥ 1

Aggregate metrics:
  Import success rate ≥ 95% (SFF1)
  Import success rate ≥ 75% (SFF2, v1.0 chấp nhận thấp hơn)
  CASM parse rate ≥ 90%
  Mean limitation count per file ≤ 3
```

---

## 6. PERFORMANCE & LATENCY TESTS (thiết bị thật)

### 6.1. Thiết bị test

- **Primary:** iPad Pro M2 11" (flagship, phải pass mọi test).
- **Secondary:** iPad Air M1 (mainstream target).
- **Stretch:** iPad mini 6 (nhỏ nhất trong target).
- iOS version: latest release + latest-1.

### 6.2. Audio latency benchmark

```
Test setup:
  - CoreAudio buffer size: 128 frames (2.9ms @ 44.1kHz) — aggressive
  - CoreAudio buffer size: 256 frames (5.8ms) — recommended live
  - CoreAudio buffer size: 512 frames (11.6ms) — fallback

Đo:
  - MIDI Note On in → chord recognized → first MIDI event out (loopback)
  Target: < 20ms total @ 256 buffer (MIDI out mode)
  Target: < 30ms total @ 256 buffer (internal sound mode)

Method: MIDI loopback test qua USB-MIDI interface
  hardware timestamp in → hardware timestamp first NoteOn out
  Run 1000 trials, report P50/P95/P99
```

### 6.3. CPU headroom benchmark

```
Scenario: 8 track style, internal sound, reverb/chorus on, mixer open
Measure: CPU% trong audio thread (Instruments profiler)
Target: < 40% CPU @ 256 buffer trên iPad Air M1
  (60% headroom cho UI, background, battery)

Section switch CPU spike: measure max CPU trong 2 buffer khi switch
Target spike: < 60% (tức là không gây xcallback overrun)
```

### 6.4. Startup & import time

```
App cold start → ready to play: < 5 giây (iPad Air M1)
Import 1 style SFF1 (average size ~100KB): < 2 giây
Import 1 style SFF1 (large ~500KB): < 5 giây
Library scan 500 style: < 10 giây (background, không block UI)
```

---

## 7. BETA / FIELD TESTING

### 7.1. Beta cohort (Tháng 6)

**20 nhạc công** qua TestFlight, profile đa dạng:
- 8 nhạc công tiệc cưới/nhà hàng VN (core persona).
- 4 nhạc công nhà thờ.
- 4 nhạc công nước ngoài (Yamaha community forum).
- 4 nhạc công semi-pro / studio.

Mỗi người: ≥ 3 buổi diễn thật (không phải chỉ test ở nhà).

### 7.2. Beta feedback protocol

**Bắt buộc:** app tự log crash (Crashlytics hoặc tương đương) — không phụ thuộc nhạc công nhớ báo lỗi.

**Sau mỗi buổi diễn:** form 5 câu ngắn:
1. Có crash/glitch không? Mô tả nếu có.
2. Style import có vấn đề gì không?
3. Section switch có lệch nhịp không (lần nào)?
4. Chord recognition có nhận sai không (hay, thỉnh thoảng, không)?
5. Điểm tổng thể 1–5, so sánh với thiết bị hiện dùng.

**Hai tuần/lần:** gặp video call nhóm, nghe phản hồi sâu hơn.

### 7.3. Tiêu chí exit beta

- ≥ 18/20 nhạc công dùng được trong show thật 2 giờ không crash.
- Chord recognition: ≥ 90% "không nhận sai" hoặc "thỉnh thoảng".
- Section switch lệch nhịp: 0 trường hợp được báo cáo trong vòng 2 tuần cuối.
- Import success: ≥ 90% style mà nhạc công mang theo import được.

---

## 8. REGRESSION TESTING

### 8.1. CI pipeline (mỗi commit)

```
[GitHub Actions / Xcode Cloud]
  1. Build debug + release
  2. Unit tests (tất cả) — target < 3 phút
  3. Integration smoke tests — target < 5 phút
  4. UASF validation trên 50 style sample
  5. Linting (SwiftLint + clang-tidy)
  Gate: tất cả phải pass trước khi merge
```

### 8.2. Nightly

```
  1. Full corpus test (500 style)
  2. Timing stress test 30 phút (simulator)
  3. Memory stability test 1 giờ (simulator)
  4. Performance benchmark trên thiết bị thật (nếu có Xcode Cloud device pool)
  5. Report: metric trends, regression alert nếu P99 timing tăng >20%
```

### 8.3. Fixture management

Mỗi bug fix phải kèm **regression test fixture** — file hoặc input sequence tái hiện bug. Commit vào repo cùng fix. Điều này đảm bảo bug không tái hiện silently trong future refactor.

---

## 9. CÔNG CỤ & INFRASTRUCTURE

| Mục đích | Công cụ | Ghi chú |
|---|---|---|
| Unit/Integration test framework | XCTest (native) | Không cần thư viện ngoài |
| C++ unit tests | GoogleTest | Link vào test target, không ship production |
| Fuzz testing | libFuzzer (tích hợp LLVM) | Chạy locally, output seeds vào repo |
| Crash reporting | Crashlytics (Firebase) | Free tier đủ cho beta |
| Performance profiling | Instruments (Xcode) | Time Profiler + Core Audio tap |
| CI | Xcode Cloud hoặc GitHub Actions + self-hosted Mac | |
| Test corpus storage | Git LFS hoặc S3 private | Style file binary, không commit vào main repo |
| MIDI loopback (latency test) | iConnectivity mio (hardware) | 1 lần mua, dùng mãi |

---

## 10. TEST KHÔNG LÀM (conscious exclusions)

- **UI automation end-to-end (XCUITest):** chi phí maintain cao, UI thay đổi nhiều trong MVP. Thêm sau khi UI stable (phase 2).
- **100% code coverage:** target không phải line coverage mà là **behavior coverage** của 3 yêu cầu phi chức năng cốt lõi.
- **Load testing marketplace/cloud:** không applicable MVP.
- **Accessibility testing:** quan trọng nhưng defer sau UI stable.

---

## 11. TRÁCH NHIỆM

| Role | Test trách nhiệm |
|---|---|
| C++ Audio Engineer | Unit test engine (transform, scheduler, state machine), timing stress |
| iOS Engineer | Unit test parser, integration tests, CI setup |
| QA (part-time từ tháng 4) | System test, beta protocol, corpus expansion |
| Domain expert (nhạc công) | A/B nhạc tính, beta cohort lead, acceptance criteria |
| Mọi người | Viết regression fixture khi fix bug |
