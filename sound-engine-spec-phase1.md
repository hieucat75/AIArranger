# SOUND ENGINE SPEC — PHASE 1
**Module 7 — Internal Sound Engine — v0.9**

---

## 1. PHẠM VI PHASE 1

Phase 1 mục tiêu: **"Chơi được standalone, nghe chấp nhận được"** — không phải "nghe như Yamaha Genos". Chất lượng sound cao là phase 2+. Phase 1 là foundation đúng kiến trúc để phase 2 cắm vào không cần rebuild.

**Deliverables phase 1:**
- sfizz integration đúng kiến trúc.
- GM soundfont bundle (GeneralUser GS hoặc tương đương).
- Mixer: volume/pan/mute/solo per track.
- Send FX: reverb + chorus (quality "nghe được", không phải "nghe tốt").
- Master limiter (tránh clip).
- Latency: < 30ms @ 256 buffer (iPad Air M1).

**Không làm phase 1:**
- Velocity layers (chỉ dùng layer default).
- Round robin.
- Release samples.
- Key switching.
- Drum kit editor.
- Disk streaming.
- User sample import UI (backend có, UI phase 2).
- Expansion pack.
- Advanced articulation.

---

## 2. SFIZZ INTEGRATION

### 2.1. Tại sao sfizz

| Criteria | sfizz | FluidSynth | Custom |
|---|---|---|---|
| License | BSD-2 ✅ | LGPL (dynamic link issue iOS) ❌ | N/A |
| SFZ support | Native ✅ | N/A | N/A |
| SF2 support | Via convert ✅ | Native ✅ | N/A |
| Real-time safety | ✅ (audio thread safe) | Partial ⚠️ | Depends |
| iOS App Store | ✅ (static link OK) | ❌ (LGPL static link issues) | N/A |
| Quality | Good ✅ | Good ✅ | N/A |
| Maintenance | Active (2024) ✅ | Active ✅ | High cost ❌ |

sfizz là lựa chọn rõ ràng. Tích hợp bằng CMake submodule.

### 2.2. Kiến trúc tích hợp

```cpp
class SfizzVoicePool {
  sfz::Synth synth;              // 1 sfizz instance per track
  int        midiChannel;
  float      volume = 1.0f;
  float      pan    = 0.0f;     // -1.0 to +1.0
  bool       muted  = false;

public:
  void loadSFZ(const std::string& path);    // gọi từ loader thread
  void noteOn(int note, int vel, int delay);  // delay = frame offset
  void noteOff(int note, int delay);
  void cc(int cc, int val, int delay);
  void renderBlock(float* L, float* R, int nFrames);
};
```

Mỗi track = 1 `SfizzVoicePool`. Drum track: load drum kit SFZ. Pitched track: load instrument SFZ.

### 2.3. SFZ file structure cho GM bank

GM bank được tổ chức thành collection SFZ files:
```
soundpacks/gm_bank/
├── drums/
│   └── standard_kit.sfz
├── bass/
│   ├── fingered_bass.sfz
│   └── picked_bass.sfz
├── piano/
│   ├── grand_piano.sfz
│   └── ...
├── strings/
├── brass/
├── choir/
└── ...
```

Mỗi SFZ file: single-layer (phase 1), root note mapping đủ để sound OK. Phase 2 thêm velocity layers.

### 2.4. SF2 → SFZ conversion

Khi user import SF2 file (hoặc bundle GM bank dạng SF2), converter offline chạy lúc import:
```
SF2 → extract SFZ mapping + WAV samples → write .sfz + .wav to app storage
```

Thư viện convert: `sfizz-render` CLI tool hoặc implement từ sf2 spec. Để tránh complexity, phase 1 bundle sẵn SFZ (không cần convert flow cho GM bank).

---

## 3. AUDIO GRAPH

```
Track 1 (Drums) ──── SfizzVoicePool ────┐
Track 2 (Bass)  ──── SfizzVoicePool ────┤
Track 3 (Chord) ──── SfizzVoicePool ────┤──→ Mixer (vol/pan) ──→ Send FX Bus
Track 4 (Pad)   ──── SfizzVoicePool ────┤         │                   │
Track 5 (Phr1)  ──── SfizzVoicePool ────┘     (per track)         Reverb
Track 6 (Phr2)  ──── SfizzVoicePool ────┘         │               Chorus
                                                   ↓                   │
                                              Master Mix ←─────────────┘
                                                   ↓
                                            Master Limiter
                                                   ↓
                                            AVAudioEngine output
```

### 3.1. Mixer

```cpp
struct TrackMixState {
  float volume;      // 0.0–1.0 (linear), maps từ CC#7 0–127
  float pan;         // -1.0 to +1.0, maps từ CC#10 0–127 → -1..+1
  float reverbSend;  // 0.0–1.0
  float chorusSend;  // 0.0–1.0
  bool  muted;
  bool  soloed;
};
```

**Solo logic:** nếu ≥ 1 track soloed → chỉ render soloed tracks. Muted track vẫn render nhưng output = 0 (để voice state maintain đúng, tránh pops khi unmute).

**Smooth parameter change:** volume/pan change dùng linear ramp 10ms để tránh click. Update từ UI thread qua atomic float.

### 3.2. Send FX — Reverb (Phase 1)

**Phase 1:** dùng JUCE `juce::dsp::Reverb` (Schroeder model, simple nhưng functional). Không phải "studio quality" nhưng nghe được live.

```cpp
juce::dsp::Reverb::Parameters reverbParams;
reverbParams.roomSize   = 0.5f;
reverbParams.damping    = 0.7f;
reverbParams.wetLevel   = 1.0f;   // send fx: wet = 100%
reverbParams.dryLevel   = 0.0f;
reverbParams.width      = 0.8f;
reverbParams.freezeMode = 0.0f;
```

Phase 2: thay bằng Airwindows `MVerb` hoặc `Verbity` (MIT, chất lượng cao hơn nhiều).

### 3.3. Send FX — Chorus (Phase 1)

JUCE `juce::dsp::Chorus`. Simple LFO-based chorus. Phase 2: nâng cấp.

### 3.4. Master Limiter

JUCE `juce::dsp::Limiter` hoặc `juce::dsp::Compressor` configured as limiter:
- Threshold: -3 dBFS.
- Release: 100ms.
- Mục đích: prevent clip, không coloring sound.

---

## 4. AUDIO SESSION SETUP (iOS)

```swift
// AppDelegate hoặc AudioSessionManager
let session = AVAudioSession.sharedInstance()
try session.setCategory(.playAndRecord,
                         mode: .default,
                         options: [.defaultToSpeaker, .allowBluetoothA2DP])
try session.setPreferredIOBufferDuration(256.0 / 44100.0)  // 256 frames @ 44.1kHz
try session.setActive(true)

// Interruption handling (phone call, Siri, etc.)
NotificationCenter.default.addObserver(
  self, selector: #selector(handleInterruption),
  name: AVAudioSession.interruptionNotification, object: nil)
```

**Interruption handling:**
- Begin interruption: pause transport, save state.
- End interruption: reactivate session, restore state, notify UI.
- Route change (headphone plug/unplug): re-query route, update UI, không dừng transport.

### 4.1. Buffer Size Strategy

| Buffer | Latency @ 44.1kHz | Tradeoff |
|---|---|---|
| 128 frames | 2.9ms | Lowest latency, highest CPU risk |
| 256 frames | 5.8ms | **Recommended default** |
| 512 frames | 11.6ms | Fallback cho device yếu |

App cho user chọn buffer size trong Settings. Default: 256. Detect CPU overrun → suggest tăng buffer.

---

## 5. SOUNDMAP → SFZ PRESET LOOKUP

Khi load UASF style, `soundMap` chứa `internalPreset` per track (vd `"bass/fingered_standard"`). Sound Engine lookup:

```cpp
std::string SoundEngine::resolveSFZPath(const std::string& internalPreset):
  // 1. User soundpack có preset này không?
  if (userPackRegistry.has(internalPreset)):
    return userPackRegistry.get(internalPreset);
  // 2. GM bank fallback
  return gmBankPath + "/" + internalPreset + ".sfz";
  // 3. Ultimate fallback: grand piano
  return gmBankPath + "/piano/grand_piano.sfz";
```

Import Adapter phải điền `internalPreset` bằng bảng lookup Yamaha Voice → GM instrument. Bảng này là deliverable riêng (Yamaha Voice Map Table) — xây dựng trong Sprint 1.

---

## 6. PERFORMANCE & LATENCY TARGET

```
Test setup: iPad Air M1, 256 buffer @ 44.1kHz, 6 track sfizz, reverb + chorus on

Metric                           Target        Stretch
──────────────────────────────────────────────────────
Audio render callback time       < 4ms         < 2ms
CPU usage (audio thread)         < 40%         < 25%
Note On → sound out latency      < 30ms        < 20ms
Memory footprint (GM bank)       < 200MB       < 100MB
No xrun in 30 min stress test    Required      —
```

---

## 7. INTERFACE (Sound Engine ↔ Arranger Engine)

```cpp
class SoundEngine {
public:
  void prepare(double sampleRate, int maxBlockSize);
  void loadTrackInstrument(TrackId, const std::string& sfzPath);  // loader thread
  void setMixState(TrackId, TrackMixState);   // UI thread, atomic

  // Gọi từ audio thread:
  void scheduleEvent(const MidiEvent&, int frameOffset);
  void renderBlock(float* outputL, float* outputR, int nFrames);

  // Mixer convenience (gọi từ UI):
  void setVolume(TrackId, float vol);
  void setPan(TrackId, float pan);
  void setMute(TrackId, bool);
  void setSolo(TrackId, bool);
  void setReverbSend(TrackId, float);
  void setChorusSend(TrackId, float);
};
```

SoundEngine là consumer của cùng event stream với MIDI Engine — Arranger Engine không cần biết output đi đâu.

---

## 8. PHASE 2 ROADMAP (không làm MVP, architecture sẵn)

| Feature | Effort | Impact |
|---|---|---|
| Velocity layers (8 layers) | M | Nghe tự nhiên hơn nhiều |
| Round robin (4 variations) | M | Tránh "machine gun" effect |
| Release samples | S | Piano tail, guitar mute |
| Disk streaming | L | Library lớn không cần toàn bộ vào RAM |
| User SFZ/SF2 import UI | M | User-owned instruments |
| Drum kit editor | M | Customize kit mapping |
| Advanced reverb (Airwindows) | S | Chất lượng FX lên rõ rệt |
| Per-track EQ + Compressor | M | Studio-grade mixing |
| Expansion pack mapping UI | L | Yamaha voice → internal preset manual |
