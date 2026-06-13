# PERFORMANCE & LATENCY REQUIREMENTS
**Live Arranger Platform — v0.9 — Formal Spec**

---

## 1. MỤC ĐÍCH

Tài liệu này là formal spec của mọi yêu cầu hiệu năng và latency. Mọi con số có: (a) ngưỡng pass/fail, (b) test method cụ thể, (c) thiết bị tham chiếu, (d) điều kiện test. Không có "cảm giác nhanh" — chỉ có số đo được.

---

## 2. THIẾT BỊ THAM CHIẾU

| Tier | Device | Role |
|---|---|---|
| **Primary** | iPad Pro M2 11" | Mọi test phải pass |
| **Target** | iPad Air M1 | Mọi test phải pass (đây là thiết bị nhạc công dùng nhiều nhất) |
| **Minimum** | iPad mini 6 (A15) | P0/P1 requirements phải pass |
| **Stretch** | iPad (9th gen, A13) | Tier P1 pass là tốt, P2 có thể fail |

iOS version: latest release tại thời điểm test.

---

## 3. LATENCY REQUIREMENTS

### 3.1. Chord Recognition Latency

**Định nghĩa:** Từ MIDI Note On packet đến từ hardware → `Chord` object được push vào `chordQueue`.

```
Requirement:  P99 ≤ 5ms
Target:       P99 ≤ 1ms
Priority:     P0

Test method:
  - 1,000 MIDI Note On events gửi qua USB MIDI loopback
  - Timestamp tại CoreMIDI callback entry
  - Timestamp tại chordQueue.push()
  - Report: P50, P95, P99, max

Pass condition: P99 ≤ 5ms trên iPad Air M1
```

### 3.2. Chord → MIDI Output Latency (External Sound Module)

**Định nghĩa:** Từ MIDI Note On (hợp âm) tới MIDI Note On của track bị retrigger ra USB MIDI output.

```
Requirement:  ≤ 20ms (end-to-end)
Target:       ≤ 12ms
Priority:     P0

Budget breakdown:
  CoreMIDI input callback:       ~0.5ms
  Chord recognition:             ≤ 1ms  (§3.1)
  Wait for next audio render:    0–5.8ms (256 buffer @ 44.1kHz)
  Audio render + transform:      < 1ms
  CoreMIDI output schedule:      ~0.5ms
  Total:                         ≤ ~9ms typical, ≤ 20ms worst case

Test method:
  - Hardware MIDI loopback: send chord NoteOn → capture first retrigger NoteOn
  - 500 trials, vary chord timing within beat
  - iConnectivity mio hoặc tương đương (hardware timestamp)

Pass condition: P95 ≤ 20ms trên iPad Air M1, buffer 256 @ 44.1kHz
```

### 3.3. Chord → Audio Output Latency (Internal Sound)

**Định nghĩa:** Từ MIDI Note On (hợp âm) tới âm thanh phát ra speaker/headphone.

```
Requirement:  ≤ 30ms
Target:       ≤ 20ms
Priority:     P0

Additional latency vs §3.2:
  sfizz voice generation:   ~1-2ms
  AVAudioEngine output:     phụ thuộc buffer (5.8ms @ 256 frames)

Test method:
  - Record audio từ headphone out
  - MIDI NoteOn timestamp (CoreMIDI) vs audio onset (Audacity/custom tool)
  - 100 trials

Pass condition: P95 ≤ 30ms trên iPad Air M1, 256 buffer @ 44.1kHz
```

### 3.4. Section Switch Timing Accuracy

**Định nghĩa:** Sai số giữa thời điểm section mới BẮT ĐẦU THỰC TẾ (tick của event đầu tiên) và thời điểm LÝ THUYẾT (cuối beat hoặc cuối bar theo spec).

```
Requirement:  ≤ 2 tick (tại 480 PPQ, 120 BPM = ~2.1ms)
Target:       ≤ 1 tick (~1ms)
Priority:     P0 — ZERO TOLERANCE cho lệch nhịp nghe được

Test method:
  - Render offline 30 phút: loop Main A, trigger Fill mỗi 4 bar, section switch mỗi 8 bar
  - Parse output MIDI, đo timestamp event đầu section mới vs expected tick
  - 1,000 switches

Pass condition: MAX sai số ≤ 2 tick (không phải P99 — bất kỳ instance nào > 5 tick = fail)
```

### 3.5. UI Tap Response

**Định nghĩa:** Từ touch down trên section button → visual state change (button highlight).

```
Requirement:  ≤ 100ms
Target:       ≤ 50ms (1 frame @ 60Hz = 16.7ms, nhưng SwiftUI có overhead)
Priority:     P1

Test method:
  - High-speed camera (240fps) recording button area
  - Touch down frame vs highlight frame
  - 50 trials

Pass condition: P95 ≤ 100ms
```

---

## 4. STABILITY REQUIREMENTS

### 4.1. Crash-free Live Session

```
Requirement:  0 crash trong 2 giờ continuous operation
Priority:     P0

Test scenario:
  - 5 style khác nhau, xoay vòng mỗi 20 phút
  - Chord changes random mỗi 2–8 beats
  - Section switches random mỗi 4–16 bars
  - Panic button mỗi 10 phút
  - MIDI device disconnect + reconnect mỗi 30 phút (nếu hardware test)
  - Background: import 10 style trong khi chơi (tháng 1 vào library)
  - Screen dimming, app backgrounding briefly (phone call sim)

Pass condition: 0 crash, 0 audio dropout > 100ms, transport không dừng
Run count: 3 lần liên tiếp (total 6 giờ test)
```

### 4.2. Note Orphan Rate

```
Requirement:  0 orphan NoteOn sau mọi test scenario
Priority:     P0

Orphan = NoteOn không có matching NoteOff trong 5 giây

Test:
  - Parse toàn bộ MIDI output của 2h session
  - Count NoteOn - NoteOff per pitch per channel
  - End of session: count phải = 0 (sau Panic được trigger cuối)
```

### 4.3. Audio Thread Deadline

```
Requirement:  0 CoreAudio deadline miss trong 1h test
Priority:     P0

Detect: CoreAudio `ioActionFlags` contains `kAudioUnitRenderAction_OutputIsSilence`
hoặc callback duration > buffer duration

Target: kAudioTimeStampSampleTimeValid, monitor via custom callback timer
Pass: 0 miss trong 1h @ 256 buffer với full CPU load scenario
```

---

## 5. THROUGHPUT REQUIREMENTS

### 5.1. Style Import Throughput

```
Requirement:  < 5 giây / style (≤ 500KB SFF1)
              < 15 giây / style (≤ 2MB SFF2 với nhiều sections)
Priority:     P1

Test:
  - 50 style SFF1, đo từ tap Import → Library entry xuất hiện
  - 20 style SFF2, đo tương tự
  - Single import (không batch)

Pass: P95 < 5 giây (SFF1), P95 < 15 giây (SFF2) trên iPad Air M1
```

### 5.2. Library Scan

```
Requirement:  500 style metadata scan (không unzip) < 10 giây
Priority:     P1

Test: Tạo library 500 style, restart app, đo từ start đến library sẵn sàng browse

Pass: < 10 giây trên iPad Air M1
```

### 5.3. Style Load Into Live

```
Requirement:  < 2 giây từ "Load to Live" → style sẵn sàng phát
Priority:     P1

Bao gồm: unzip .uas, parse manifest, load SFZ files vào sfizz, pre-compute NTT table

Test: 20 style, đo từ tap → first Main A event emitted
Pass: P95 < 2 giây (iPad Air M1)
```

---

## 6. RESOURCE USAGE

### 6.1. CPU

```
Scenario: 8 tracks active, internal sound (sfizz), reverb + chorus, MIDI out
Measure: Instruments Time Profiler, audio thread CPU%

Requirement:  ≤ 50% audio thread CPU @ 256 buffer (iPad Air M1)
Target:       ≤ 35%
Priority:     P1

Rationale: 50% headroom cho UI thread, background import, battery efficiency
```

### 6.2. Memory

```
Baseline (app idle, no style loaded):           ≤ 80MB
Style loaded (GM bank + 1 UASF):               ≤ 200MB
Style loaded + 5 preloaded setlist songs:       ≤ 300MB
After 2h session (leak check):                  ≤ baseline × 1.1

Priority: P1
Test: Xcode Memory Debugger, Instruments Allocations
```

### 6.3. Battery

```
Scenario: Screen on, MIDI out only (no internal sound), style playing
Requirement:  ≤ 15% battery/hour (iPad Air M1)

Scenario: Screen on, internal sound (sfizz), reverb on
Requirement:  ≤ 25% battery/hour

Priority: P1
Test: iPad battery % before/after 1h session
```

### 6.4. Storage

```
App install size (without GM bank):   ≤ 80MB
App install size (with GM bank):      ≤ 200MB
Per .uas style (average):             50–300KB
Library 500 style:                    25–150MB

Priority: P2
```

---

## 7. PERFORMANCE BUDGET — SPRINT CHECKPOINTS

Dùng để detect regression sớm trong CI:

| Sprint | Checkpoint | Metric | Pass threshold |
|---|---|---|---|
| S0 | Spike: chord recognition | Chord latency P99 | ≤ 10ms (relaxed cho prototype) |
| S1 | MIDI output pipeline | End-to-end latency P95 | ≤ 25ms |
| S2 | Section switch | Timing error MAX | ≤ 5 tick |
| S3 | Full system | All P0 requirements | Pass |
| Beta | Live session | 2h stability × 3 | Pass |

---

## 8. DEGRADATION POLICY

Khi thiết bị không đạt được target (vd iPad cũ hơn):

| Symptom | Auto action | User notification |
|---|---|---|
| Audio callback > 80% duration | Suggest tăng buffer size | Toast: "Cân nhắc tăng buffer trong Settings" |
| 3 deadline misses trong 1 phút | Auto tăng buffer size | Dialog: "Đã tăng buffer để ổn định" |
| Memory > 400MB | Unload preloaded songs | Silent |
| Import > 30 giây | Cancel + error | "Import quá lâu — file có thể bị lỗi" |

---

## 9. TEST AUTOMATION

Mọi P0 requirement phải có automated test trong CI:

| Requirement | Test type | CI frequency |
|---|---|---|
| Chord latency | Unit benchmark (offline render) | Every commit |
| Section timing | Offline render + MIDI parse | Every commit |
| Note orphan | Offline render 30 min | Nightly |
| Crash-free 2h | System test | Weekly (manual trigger) |
| Memory stability | Instruments script | Nightly |
| Import throughput | Benchmark với corpus | Nightly |
