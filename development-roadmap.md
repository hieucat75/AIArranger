# DEVELOPMENT ROADMAP & SPRINT PLANS
**Live Arranger Platform — v0.9**

---

## 1. ROADMAP TỔNG QUAN (12 THÁNG)

```
Month:    1        2        3        4        5        6
          ├────────┼────────┼────────┼────────┼────────┤
Sprint:   [S0     ][S1     ][S2     ][S3     ][S4     ][BETA   ]
Focus:    Foundation  Engine   I/O    Product  Polish   Field

Month:    7        8        9        10       11       12
          ├────────┼────────┼────────┼────────┼────────┤
          [Launch ][Phase2 start    ][AUv3+Sound Eng  ][Korg   ]
```

### Milestones

| Milestone | Mô tả | Target |
|---|---|---|
| **M0** | Spike: SFF1 parse + Main A phát qua MIDI out | Cuối tuần 2 S0 |
| **M1** | Chord nhận → style transform đúng → MIDI out | Cuối S1 |
| **M2** | Full arranger: section switch, fill, ending — đúng nhịp | Cuối S2 |
| **M3** | Full product: sound, mixer, UI, setlist | Cuối S4 |
| **M4** | Beta: 20 nhạc công dùng show thật 2h | Tháng 6 |
| **M5** | App Store launch | Tháng 8 |

---

## 2. SPRINT 0 — FOUNDATION (Tuần 1–4)

**Mục tiêu:** De-risk mọi technical unknown trước khi build. Nếu Sprint 0 không xong M0, dừng và đánh giá lại team/approach.

**Thành công Sprint 0:**
- Spike M0 pass (xem §2.1).
- UASF spec và Technical Architecture chốt v1.0.
- CI setup, test harness chạy được.
- Test corpus 50 style đa dạng.
- Team đã đọc và sign-off mọi spec.

### 2.1. Spike M0 (Tuần 1–2) — PRIORITY ĐẦU TIÊN

```
Task: "Bấm play → nghe nhạc từ style Yamaha"

Deliverable:
  Input:  1 file .sty SFF1
  Output: Main A phát qua CoreMIDI → external MIDI sound module

Steps:
  Day 1–2:  Đọc JJazzLab YamJJazz source (LGPL, tham khảo thuật toán)
            Hex dump 5 style file, map cấu trúc với spec §3–6 của File Import Strategy
  Day 3–5:  Viết minimal SFF1 parser (C++): detect magic, parse track, extract Main A events
  Day 6–7:  Viết minimal UASF builder: manifest.json + main_a.mid
  Day 8–9:  Viết minimal ArrangerEngine: load UASF, phát events theo audio clock
  Day 10:   Connect CoreMIDI output, test với sound module thật

Pass criteria:
  - Main A phát đúng BPM và note sequences (so sánh bằng tai + MIDI monitor)
  - Không crash trong 5 phút liên tục
  - Log rõ mọi parse error
```

### 2.2. Spike Audio Stack (Tuần 1–2, song song)

```
Task: "sfizz + JUCE phát âm thanh ra iPad speaker"

Steps:
  Day 1–3:  Setup JUCE project, link sfizz, configure CMake
  Day 4–5:  Load GeneralUser GS soundfont (SFZ), phát C major chord
  Day 6–7:  Đo latency: bấm virtual NoteOn → âm thanh → target < 30ms
  Day 8:    Đo CPU @ 256 buffer với 6 tracks active

Pass criteria:
  - Âm thanh phát không glitch trong 10 phút
  - Latency P95 < 35ms (relaxed cho spike)
  - CPU < 60% (relaxed)
```

### 2.3. Tuần 3–4: Infrastructure & Spec Finalize

```
Week 3:
  - CI setup (Xcode Cloud hoặc GitHub Actions + Mac mini)
  - Unit test framework: XCTest + GoogleTest linked
  - Timing stress test harness (offline render + MIDI parse)
  - SQLite + GRDB setup, schema v1 migration
  - Style corpus thu thập: nhờ beta team export từ keyboard họ sở hữu

Week 4:
  - UASF Spec v1.0-rc: review dựa trên spike findings
  - Technical Architecture v1.0: update ADR từ spike learnings
  - File Import Strategy: điền bit-level offsets từ hex dump + JJazzLab cross-ref
  - Chord Engine test harness: 1,224 test case generate + run
  - Sprint 1 planning: task breakdown, estimates, acceptance criteria

Deliverables Sprint 0:
  ✓ Spike M0 pass (bắt buộc)
  ✓ Spike Audio pass (bắt buộc)
  ✓ CI pipeline (required)
  ✓ 50+ style test corpus (required)
  ✓ UASF Spec v1.0-rc (required)
  ✓ Chord Engine unit tests (1224 cases) green (required)
```

---

## 3. SPRINT 1 — ENGINE CORE (Tháng 2)

**Mục tiêu:** Milestone M1 — "Bấm hợp âm → style transform → MIDI out đúng hòa âm"

### Week 1–2: SFF1 Import Adapter

```
Tasks:
  - SFF1 parser hoàn chỉnh: tất cả sections, CASM/Ctab đầy đủ
  - Validation V01–V07
  - Import result với limitation codes
  - UASF writer: zip container, manifest.json, section .mid files
  - Test: 50 style corpus → report success rate (target ≥ 95%)
  - SFF2 basic: detect format, parse MIDI sections (CASM có thể skip → log CASM_MISSING)

Acceptance:
  - Import 50 style SFF1: ≥ 47 success (94%)
  - Limitation codes đúng và human-readable
  - Không crash trên bất kỳ file nào trong corpus (fail gracefully)
```

### Week 2–3: Chord Engine

```
Tasks:
  - Lookup table generator (Python script → generate C++ header)
  - ChordEngine C++ class: pushNote, recognize, lookup, partial match
  - InputRouter: split point, chordQueue, rightHandQueue
  - Fingered mode: exact + partial match
  - Single Finger mode: Yamaha convention
  - Hold mode, chord latch 80ms
  - Manual bass mode
  - All 1,224 unit tests green

Acceptance:
  - Unit tests 100% pass
  - Latency P99 < 2ms (spike target strict)
  - Fuzz test 100k random inputs: 0 crash
```

### Week 3–4: Arranger Engine Core

```
Tasks:
  - MusicalClock: audio callback clock, ticksPerSample, blockStartTick/End
  - SectionStateMachine: STOPPED/PLAYING, pending, boundary detection
  - Section switch quantize: fill=beat, main=bar, ending=bar
  - NTR transform: rootTransposition, rootFixed
  - NTT transform: bypass, melody, chord, bass, melodicMinor, harmonicMinor
  - ActiveNotes tracking
  - Retrigger: stop, pitchShift, pitchShiftToRoot, retrigger, retriggerToRoot
  - Note limit wrap
  - Panic: flush queue + emit all NoteOff

Acceptance (M1):
  - Bấm Cm7 → bass track phát đúng transformation (NTT bass mode)
  - Section switch không lệch nhịp (timing test: P99 < 2 tick)
  - Note orphan: 0 sau 30 phút offline render
  - Chord change → retrigger ngay tại frame đúng
```

---

## 4. SPRINT 2 — I/O COMPLETE (Tháng 3)

**Mục tiêu:** Milestone M2 — Full arranger pipeline hoạt động đầy đủ

### Week 1–2: MIDI Engine

```
Tasks:
  - MidiEngine C++ class với CoreMIDI integration
  - Device enumeration, hot-plug notification
  - Per-track routing table
  - Initialization sequence (GM/GS/XG)
  - Active note tracking, panic
  - Virtual MIDI port (outbound)
  - MIDI clock out (24 PPQN, sample-accurate)
  - MIDI input: InputRouter, chordQueue bridge

Acceptance:
  - USB MIDI out: events delivered với correct MIDITimeStamp
  - Hot-plug: device disconnect không crash, không dừng transport
  - Virtual MIDI: AUM hoặc GarageBand nhận được events
```

### Week 2–3: Sound Engine Phase 1

```
Tasks:
  - sfizz integration hoàn chỉnh: 1 instance per track, noteOn/Off/cc
  - Audio graph: per-track mix → send FX → master limiter
  - JUCE Reverb + Chorus send FX
  - Mixer: volume/pan/mute/solo atomic
  - GM bank bundle (GeneralUser GS SFZ)
  - soundMap → SFZ preset lookup table
  - AVAudioSession setup + interruption handling

Acceptance:
  - Internal sound: latency P95 < 30ms @ 256 buffer
  - CPU < 50% (iPad Air M1, 6 tracks, reverb on)
  - 10 phút không xrun
  - Solo/mute toggle: no click/pop
```

### Week 3–4: SFF2 + Full System Integration

```
Tasks:
  - SFF2 Ctb2 parser: multi-range noteRanges, NTT extended enum
  - SFF2 limitations: guitar NTR fallback, megavoice skip
  - End-to-end integration test: SFF1/SFF2 → UASF → Arranger → MIDI+Sound
  - Section state machine: Intro, Main A-D, Fill AA-DD, Break, Ending 1-3
  - Auto fill, count-in, sync start/stop
  - Tap tempo (4-tap average)
  - MIDI clock in research (không implement, chỉ interface stub)

Acceptance (M2):
  - Chơi 30 phút liên tục: 0 xrun, 0 orphan note, timing P99 < 2 tick
  - SFF2: ≥ 75% import success với corpus
  - Full section state machine: tất cả integration tests green
```

---

## 5. SPRINT 3 — PRODUCT (Tháng 4)

**Mục tiêu:** App đầy đủ tính năng để nhạc công dùng được

### Week 1–2: SwiftUI — Live Screen + Mixer

```
Tasks:
  - Live Screen layout (landscape + portrait)
  - Section pad grid: states (default/active/pending), animation
  - Chord display: font lớn, cross-fade animation
  - Bottom bar: transport, tempo (tap-to-edit), transpose, bar/beat
  - Panic button: 88×88pt, always visible
  - Mixer panel: collapsible, per-track fader/pan/mute/solo
  - Performance mode: 3-finger toggle, gesture lock
  - SwiftUI ↔ ArrangerBridge: commandQueue, EngineState poll 60Hz

Acceptance:
  - Section tap: visual response < 100ms (high-speed camera test)
  - Pending animation: correct 0.8Hz pulse
  - Performance mode: cannot accidentally leave Live Screen
```

### Week 2–3: Library & Style Browser

```
Tasks:
  - SQLite schema v1, GRDB setup, migrations
  - LibraryManager: import → save, scan, query
  - Style Browser: list view, search (FTS5), filter, sort, favorite
  - Import flow UX: multi-select picker, progress, result sheet, warnings
  - Style detail: sections list, limitation detail, preview button
  - Preview: phát Main A 4 bar, stop auto

Acceptance:
  - Search 847 styles: < 200ms response
  - Import 20 styles batch: không block UI
  - Favorites persist across restart
```

### Week 3–4: Setlist + Performance

```
Tasks:
  - Setlist: create, add song, assign style/tempo/transpose, reorder drag
  - Song detail sheet: all properties editable
  - Next-song preload: load N+1 style vào RAM khi đang chơi N
  - Performance save/load: all engine state → JSON
  - Export/import performance JSON
  - On-screen chord pad: grid layout, tap root+type → chord

Acceptance:
  - Setlist 20 songs: create và navigate < 30 giây
  - Next song load: < 1 giây (đã preload)
  - Performance restore: all settings correct sau restart
```

---

## 6. SPRINT 4 — POLISH & BETA PREP (Tháng 5)

```
Week 1: Bug fixing từ internal testing
  - Fix mọi crash phát hiện
  - Fix timing issues từ extended test
  - Fix UI bugs từ usability review
  - Latency tuning: optimize buffer, sfizz config

Week 2: Polish
  - Haptic feedback implementation
  - Error handling UX: tất cả error states
  - Onboarding: demo style bundle, tooltip overlay
  - Dark mode refinement: contrast, sân khấu conditions
  - App icon + launch screen

Week 3: Beta prep
  - TestFlight setup, internal testing group
  - Crashlytics integration (opt-in)
  - 2h stability test × 3 runs (internal)
  - Corpus test 500 styles: report
  - Performance benchmark: latency, CPU, memory

Week 4: Beta distribution
  - TestFlight invite 20 nhạc công (chia theo persona)
  - Feedback form setup
  - Week 1 beta check-in

Acceptance (M3 — App):
  - Tất cả P0/P1 requirements from PRD pass
  - 2h stability: pass × 3 consecutive runs
  - Import corpus 500 style: ≥ 95% SFF1, ≥ 75% SFF2
  - Latency: P95 ≤ 20ms MIDI out, ≤ 30ms internal
```

---

## 7. THÁNG 6 — BETA FIELD TEST

```
Week 1–2:
  - Theo dõi Crashlytics daily
  - Collect feedback forms (sau mỗi show)
  - Fix P0 bugs phát hiện từ field ngay trong ngày

Week 3–4:
  - Video call nhóm beta (round 1): nghe phản hồi sâu
  - Fix P1 bugs
  - Second TestFlight build
  - Final stability test: 3 runs × 2h

Exit criteria:
  ≥ 18/20 nhạc công: show 2h không crash
  ≥ 90% chord recognition satisfaction
  0 section switch sai nhịp trong 2 tuần cuối
  ≥ 90% style import success (beta corpus)
```

---

## 8. THÁNG 7–8 — APP STORE LAUNCH

```
Tháng 7:
  - App Store submission prep: screenshots, description, keywords
  - Trademark review (luật sư)
  - Privacy Policy + ToS publish
  - Press kit: video demo, screenshots
  - Submit App Store (expect 1–2 tuần review)
  - Marketing: YouTube video, Facebook group post, forum

Tháng 8:
  - App Store launch
  - Monitor reviews, respond to issues
  - Hotfix build nếu cần (week 1 post-launch critical)
  - Begin Phase 2 planning: AUv3 hosting, SFF2 advanced, user sample UI
```

---

## 9. DEPENDENCY & CRITICAL PATH

```
Critical path (mọi task trên path này delay = delay milestone):

SFF1 Parser (S1W1) → UASF Builder (S1W1) → Arranger Engine (S1W3)
                                                    │
Chord Engine (S1W2) ──────────────────────────────→ M1 (end S1)
                                                    │
MIDI Engine (S2W1) → ─────────────────────────────→ M2 (end S2)
Sound Engine (S2W2) ─┘
                                                    │
Live Screen UI (S3W1) → Library (S3W2) → Setlist (S3W3) → M3 (end S4)

Not on critical path (có thể delay không ảnh hưởng MVP):
  - Virtual MIDI port (S2)
  - SFF2 advanced (Ctb2) (S2W4)
  - On-screen chord pad (S3W4)
  - Export/import performance (S3W4)
```
