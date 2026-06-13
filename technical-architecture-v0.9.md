# TECHNICAL ARCHITECTURE DOCUMENT
**Live Arranger Platform — iOS/macOS — v0.9**

---

## 1. MỤC ĐÍCH

Tài liệu này là "bản đồ" kỹ thuật toàn hệ thống: cách các module kết nối, lý do chọn stack, các ranh giới thread, quy ước giao tiếp, và quyết định kiến trúc quan trọng kèm trade-off. Đây là tài liệu mọi engineer phải đọc trước khi đụng vào codebase.

---

## 2. NGUYÊN TẮC KIẾN TRÚC

**P1 — Audio thread là thánh địa:** Không malloc, không lock, không file IO, không ObjC/Swift ARC, không exception. Vi phạm = glitch nghe thấy trên sân khấu.

**P2 — Runtime không biết Yamaha tồn tại:** Import Adapter convert offline. Arranger Engine chỉ nói UASF. Thêm format mới = viết thêm Adapter, không sửa engine.

**P3 — UI không block, engine không chờ:** Giao tiếp hai chiều qua lock-free queue + atomic snapshot. UI poll 60Hz, không callback vào audio thread.

**P4 — Fail loud, degrade gracefully:** Import lỗi → báo rõ, không silent corrupt. Runtime lỗi (device disconnect, CPU spike) → degrade có kiểm soát, không crash.

**P5 — Build platform, không build app:** Internal format, plugin interface, pack metadata — tất cả thiết kế từ đầu để marketplace, AI, và Korg/Roland adapter là phần mở rộng tự nhiên.

---

## 3. KIẾN TRÚC TỔNG THỂ

```
┌──────────────────────────────────────────────────────────┐
│  PRESENTATION LAYER (SwiftUI, Main Thread)               │
│                                                          │
│  LiveScreen  MixerView  StyleBrowser  SetlistView        │
│  ImportFlow  SettingsView  LibraryView  ErrorView        │
│       │           │            │             │           │
│       └───────────┴────────────┴─────────────┘           │
│                         │                                │
│              AppStateStore (ObservableObject)            │
│              EngineStateSnapshot (atomic poll 60Hz)      │
└─────────────────────────┬────────────────────────────────┘
                          │ Command objects (value types)
                          │ Swift/C++ interop boundary
┌─────────────────────────▼────────────────────────────────┐
│  BRIDGE LAYER (Swift/C++ Interop, Xcode 15+)             │
│                                                          │
│  ArrangerBridge  MidiBridge  ImportBridge  LibraryBridge │
│  (thin wrappers, no logic — just marshal types)          │
└──────┬──────────────────────────────────────────┬────────┘
       │ C++ calls                                │ C++ calls
┌──────▼──────────────────────────────────────────▼────────┐
│  CORE ENGINE LAYER (C++17, no exceptions in audio path)  │
│                                                          │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  ArrangerEngine                                     │ │
│  │  ┌────────────┐ ┌────────────┐ ┌─────────────────┐ │ │
│  │  │MusicalClock│ │SectionState│ │NTR/NTT Transform│ │ │
│  │  │(audio-clk) │ │Machine     │ │Pipeline         │ │ │
│  │  └────────────┘ └────────────┘ └─────────────────┘ │ │
│  │  ┌────────────────────────────────────────────────┐ │ │
│  │  │  Lookahead Scheduler + Lock-free Event Queue   │ │ │
│  │  └────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                          │
│  ┌──────────────┐   ┌──────────────┐   ┌─────────────┐  │
│  │ ChordEngine  │   │  MidiEngine  │   │ SoundEngine │  │
│  │ (lookup tbl) │   │ (router)     │   │ (sfizz+DSP) │  │
│  └──────┬───────┘   └──────┬───────┘   └──────┬──────┘  │
└─────────┼──────────────────┼──────────────────┼──────────┘
          │                  │                  │
┌─────────▼──────────────────▼──────────────────▼──────────┐
│  PLATFORM ADAPTER LAYER                                  │
│  CoreMIDI   AVAudioEngine/CoreAudio   FileSystem/SQLite  │
└──────────────────────────────────────────────────────────┘
          │                  │
┌─────────▼──────────────────▼──────────────────────────────┐
│  OFFLINE / BACKGROUND LAYER (không phải audio thread)    │
│  ImportAdapter(SFF1/SFF2→UASF)   LibraryManager(SQLite)  │
│  PackManager   BackupRestore      AIInterface(phase2)     │
└───────────────────────────────────────────────────────────┘
```

---

## 4. TECHNOLOGY STACK

### 4.1. Quyết định đã chốt

| Layer | Technology | Lý do | Trade-off chấp nhận |
|---|---|---|---|
| UI | SwiftUI | Native, declarative, không lựa chọn tốt hơn trên iOS | Learning curve cho animation phức tạp |
| Audio/MIDI backbone | JUCE 7 (Indie license) | Tiết kiệm 3–6 tháng vs tự viết; DSP module, AudioProcessorGraph, cross-platform path | License phí ~$40/tháng; binary size tăng ~5MB; App Store đã ổn |
| Core engine | C++17 | Performance, predictable allocation, không GC pause | Bridge complexity với Swift |
| Swift↔C++ bridge | Swift/C++ interop (Xcode 15+) | Chính thức Apple, không cần Objective-C wrapper | API surface phải giữ mỏng |
| MIDI I/O iOS | CoreMIDI (native, bypass JUCE wrapper) | MIDITimeStamp scheduling tốt hơn JUCE MIDI trên iOS; BLE MIDI, Network MIDI native | Phải viết wrapper cho hot-plug notification |
| Audio session | AVAudioEngine (host) + CoreAudio (low-level) | AVAudioEngine manage session/routing; CoreAudio render callback | Hai level cùng lúc cần careful setup |
| Sampler | sfizz (BSD-2 license) | Production-ready SFZ player; tốt hơn FluidSynth về license iOS | Thiếu built-in SF2 loader → convert SF2→SFZ khi import |
| DSP/FX | JUCE DSP module + Airwindows (MIT) | JUCE: EQ/Compressor/Limiter chất lượng tốt; Airwindows: reverb tuyệt vời free | Airwindows code style cũ, cần wrap |
| Library metadata | SQLite + GRDB (Swift) | Proven, lightweight, dễ migrate | Không cần full CoreData complexity |
| Style/Project metadata | JSON (manifest.json trong .uas) | Human-readable, tooling sẵn, versioning dễ | Không binary efficient → hybrid với SMF giải quyết |
| MIDI events trong style | SMF (Standard MIDI File) | Binary compact, tooling sẵn, debuggable bằng DAW | Cần custom reader, không raw bytes |

### 4.2. Quyết định defer

| Technology | Lý do defer | Timeline |
|---|---|---|
| AUv3 hosting (chạy plugin) | Tốn 1–2 tháng implement + test, không unblock MVP | Phase 2 |
| CoreML / ONNX (AI local) | Chỉ declare interface; không có AI feature MVP | Phase 2+ |
| CloudKit / backend | Không có marketplace MVP | Phase 3 |
| Windows/Android port | C++ core đã chuẩn bị, nhưng chưa cần | Phase 3+ |
| Catalyst (macOS) | Chạy được ngay sau iOS done; nhưng optimize UX macOS cần effort | Phase 2 |

---

## 5. MODULE DEPENDENCY MAP

```
ImportAdapter ──────────────────────────────┐
     │                                      │
     ▼ produces                             │
  UASFStyle (data model)                    │
     │                                      │
     ▼ consumed by                          │
ArrangerEngine ◄── ChordEngine              │
     │                                      │
     ▼ produces events                      │
MidiEngine ──────────────────────────────►  CoreMIDI (out)
     │                                      │
     └──────────────────────────────────►  SoundEngine
                                            │
                                            ▼
                                        AVAudioEngine / CoreAudio
```

**Dependency rules (không được vi phạm):**
- `ArrangerEngine` không import bất kỳ header nào của ImportAdapter.
- `MidiEngine` không import header của ChordEngine.
- `SoundEngine` không biết về `UASFStyle` — chỉ nhận raw MIDI events.
- Tất cả Core Engine modules không import SwiftUI, Foundation, hay bất kỳ Apple framework nào (để portable về macOS native và tương lai Windows).
- Apple frameworks chỉ được import trong Platform Adapter Layer và Bridge Layer.

---

## 6. THREAD MODEL

```
Thread                   Nguồn              Giao tiếp ra ngoài
────────────────────────────────────────────────────────────────
Main (UI) Thread         SwiftUI            commandQueue.push (SPSC, lock-free)
                                            EngineState.snapshot() (atomic read)

CoreMIDI Callback        CoreMIDI           chordQueue.push (lock-free)
(high-priority thread)   (hardware clock)   inputEventQueue.push

Audio Render Callback    CoreAudio          outputEventQueue.push
(real-time thread)       (audio clock)      EngineState atomic write
                                            MIDI send queue push

MIDI Send Thread         audio render out   MIDISend() to CoreMIDI
(high-priority)          queue              (separate từ audio thread vì
                                            MIDISend không real-time safe)

Background Import Thread DispatchQueue.low  LibraryManager.addStyle()
                                            (sau khi import xong)

Background Library Thread DispatchQueue     SQLite read/write
```

**Lock-free queues sử dụng:**
- `commandQueue`: Main → AudioRender (SPSC)
- `chordQueue`: CoreMIDI callback → AudioRender (SPSC)
- `outputEventQueue`: AudioRender → MIDI Send Thread (SPSC)
- `soundEventQueue`: AudioRender → Sound Engine audio callback (nếu tách thread)

Tất cả queue dùng implementation đã test (folly::ProducerConsumerQueue reference, hoặc tự viết ring buffer đơn giản — không dùng `std::queue` với mutex).

---

## 7. DATA FLOW — CHORD BẤMM → ÂM THANH PHÁT

```
1. Nhạc công bấm C-E-G trên MIDI keyboard
   → CoreMIDI callback nhận 3 NoteOn packet với MIDITimeStamp

2. InputRouter phân loại: note < splitPoint → ChordEngine
   ChordEngine.pushNote() × 3
   → recognize() → Chord{root=0, type=maj, bassNote=60}
   → chordQueue.push(chord, timestamp)

3. Audio render callback (kế tiếp, ~1–3ms sau):
   a. drain chordQueue → state.chord = CMaj, timestamp = T
   b. applyRetrigger: bass track đang vang Bb → pitchShiftToRoot → emit NoteOff(Bb)+NoteOn(C) tại frame(T)
   c. advance clock: blockStartTick..blockEndTick
   d. render events từ Main A section MIDI trong window này:
      - DrumTrack: emit nguyên (isRhythm=true)
      - BassTrack: transform(srcNote, CMaj) → emit NoteOn tại frame offset
      - ChordTrack: chordMute? → nếu không → transform → emit
   e. emit tất cả vào outputEventQueue với hostTime

4. MIDI Send Thread:
   drain outputEventQueue
   → MIDISend(outputPort, endpoint, packetList) với MIDITimeStamp = hostTime
   → CoreMIDI deliver chính xác tại hardware timestamp
   → Sound module phát âm thanh

Tổng pipeline khoảng 5–15ms từ bấm đến phát (buffer 256 @ 44.1kHz = 5.8ms + CoreMIDI jitter ~1ms).
```

---

## 8. UASF STYLE — MEMORY MODEL

```cpp
// Load một lần, immutable sau đó, shared_ptr
struct UASFStyle {
  StyleManifest        manifest;    // parsed từ manifest.json
  vector<SectionData>  sections;    // mỗi section: sorted event array pre-loaded

  // pre-computed lúc load, dùng trong render loop:
  NTTLookupTable       nttTable;    // [mode][chordType][srcPC] → dstPC
  DrumNoteMap          drumMap;
};

struct SectionData {
  SectionId   id;
  SectionType type;
  int         lengthTicks;
  bool        loop;
  // Events sorted by tick, Note On/Off paired với pointer
  vector<MidiEvent> events;        // pre-allocated, immutable
};
```

Style load: background thread parse → build `UASFStyle` → atomic swap con trỏ vào `ArrangerEngine`. Old style: RCU-style deferred free sau khi audio thread confirm đã đọc version mới.

Không bao giờ load/parse trong audio thread. Không `new`/`delete` trong audio thread.

---

## 9. SOUND ENGINE ARCHITECTURE

```
ArrangerEngine output
       ↓
SoundEngine.scheduleEvent(MidiEvent, hostTime)
       ↓ (scheduled into audio render buffer)
Per-track Voice Pool (sfizz instances, 1 per track)
       ↓
Track AudioBuffer
       ↓
Insert FX Chain (per track): EQ → Comp (phase 2)
       ↓
Mixer (volume/pan/mute/solo per track)
       ↓
Send FX Bus: Reverb (zita-rev1) + Chorus (JUCE Chorus)
       ↓
Master Bus: Limiter (JUCE)
       ↓
AVAudioEngine output node → speaker/headphone
```

**sfizz integration:** 1 sfizz instance per track (sfizz is multi-voice internally). SFZ file per track loaded at style load time. Runtime: sfizz.noteOn/noteOff/cc called trong audio render thread — sfizz là real-time safe.

**GM Soundfont MVP:** dùng 1 SFZ-converted GM bank (GeneralUser GS hoặc tương đương, free license) làm fallback khi style không có soundMap match với user's pack.

---

## 10. FILE SYSTEM & LIBRARY

```
App Documents/
├── library.db              ← SQLite, quản lý bởi GRDB
├── styles/
│   ├── {uuid}.uas          ← UASF packages
│   └── ...
├── soundpacks/
│   ├── gm_bank/            ← built-in GM SFZ
│   └── {packId}/           ← user-installed packs
├── performances/
│   └── {uuid}.json         ← Performance preset (routing + settings)
├── setlists/
│   └── {uuid}.json
└── temp/                   ← import staging, cleaned on startup
```

**SQLite schema (simplified):**
```sql
CREATE TABLE styles (
  id TEXT PRIMARY KEY,       -- uuid
  name TEXT,
  genre TEXT,                -- JSON array
  tags TEXT,                 -- JSON array
  filePath TEXT,
  importedAt INTEGER,
  sourceFormat TEXT,
  warningCount INTEGER,
  isFavorite INTEGER DEFAULT 0,
  lastUsedAt INTEGER
);

CREATE TABLE performances (
  id TEXT PRIMARY KEY,
  name TEXT,
  styleId TEXT REFERENCES styles(id),
  tempoBpm REAL,
  transpose INTEGER,
  routingJson TEXT,          -- JSON blob
  chordEngineJson TEXT,
  mixerJson TEXT
);

CREATE TABLE setlists (
  id TEXT PRIMARY KEY,
  name TEXT,
  songsJson TEXT             -- ordered JSON array of performance IDs
);
```

Full-text search trên name/tags bằng SQLite FTS5.

---

## 11. CROSS-PLATFORM PATH (iOS → macOS)

C++ core engine không dùng bất kỳ Apple framework nào → portable nguyên vẹn.

```
Phase 1 (hiện tại):
  SwiftUI (iOS) → Bridge → C++ Core → CoreMIDI + CoreAudio

Phase 2 macOS (Catalyst):
  SwiftUI chạy trên Catalyst, phần lớn UI tái sử dụng.
  Bridge giữ nguyên. C++ Core giữ nguyên.
  Chỉ thêm: macOS audio session (AVAudioEngine trên macOS khác iOS nhẹ).
  Thêm: AU plugin hosting (macOS MainStage, Logic integration).

Phase 3 macOS native (nếu Catalyst không đủ):
  AppKit shell mỏng thay SwiftUI cho các màn hình desktop-specific.
  C++ Core: giữ nguyên 100%.
  Bridge: viết lại lớp mỏng cho AppKit.
```

**JUCE trên macOS:** JUCE hỗ trợ macOS đầy đủ, license Indie cover cả iOS + macOS.

---

## 12. SECURITY & PRIVACY

- **Không network request nào ở MVP.** App hoàn toàn offline.
- **Không analytics, không tracking.** Thêm opt-in analytics (crash + usage) sau khi có Privacy Policy rõ ràng.
- **File access:** chỉ App Documents + user-selected files (File.app picker). Không require full disk access.
- **MIDI permission:** request khi cần, explain rõ mục đích.
- **Crash reporting (beta):** Crashlytics với IP anonymization, opt-in trong onboarding beta.
- **DRM placeholder:** pack metadata có `license` field, validation infrastructure sẵn cho marketplace phase 3. MVP không enforce DRM.

---

## 13. BUILD CONFIGURATION

```
Targets:
  LiveArranger (iOS)          ← production
  LiveArrangerTests (iOS)     ← unit + integration
  LiveArrangerUI (iOS)        ← SwiftUI previews
  CoreEngineTests (macOS)     ← C++ unit tests via GoogleTest, chạy nhanh trên CI Mac

Schemes:
  Debug:   assertions on, sanitizers (ASan + TSan), verbose logging
  Release: assertions off, optimizations O2, minimal logging

C++ flags (release):
  -O2 -ffast-math             ← audio performance
  -fno-exceptions             ← trong audio code paths
  -DNDEBUG

CI:
  Xcode Cloud hoặc GitHub Actions + self-hosted Mac mini M-series
```

---

## 14. DEPENDENCY MANAGEMENT

| Dependency | Source | License | Integrate via |
|---|---|---|---|
| JUCE 7 | juce.com | Indie (paid) | CMake submodule / Projucer |
| sfizz | github.com/sfztools/sfizz | BSD-2 | CMake submodule |
| Airwindows (subset) | github.com/airwindows/airwindows | MIT | Copy relevant .cpp files |
| GRDB.swift | github.com/groue/GRDB.swift | MIT | Swift Package Manager |
| GoogleTest | github.com/google/googletest | BSD-3 | CMake (test targets only) |

Không dùng CocoaPods. SPM cho Swift deps, CMake cho C++ deps, manual cho JUCE (JUCE có CMake support riêng).

---

## 15. ARCHITECTURE DECISION RECORD (ADR) TÓM TẮT

| ADR | Quyết định | Ngày chốt |
|---|---|---|
| ADR-001 | iPad-first, không macOS-first | Sprint 0 |
| ADR-002 | JUCE cho audio/MIDI backbone | Sprint 0 |
| ADR-003 | MIDI output trước internal sound | Sprint 0 |
| ADR-004 | UASF = zip container (.uas) với JSON + SMF hybrid | Sprint 0 |
| ADR-005 | Audio clock là nguồn thời gian duy nhất | Sprint 0 |
| ADR-006 | noteRanges là mảng từ v1.0 để sẵn SFF2 | Sprint 0 |
| ADR-007 | sfizz thay FluidSynth (license iOS) | Sprint 0 |
| ADR-008 | Swift/C++ interop thay Objective-C wrapper | Sprint 0 |
| ADR-009 | CoreMIDI trực tiếp (bypass JUCE MIDI wrapper) | Sprint 0 |
| ADR-010 | SQLite + GRDB cho library metadata | Sprint 0 |

Mỗi ADR có document riêng ghi: context, options considered, decision, consequences. Lưu trong `docs/adr/` trong repo.
