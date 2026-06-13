# CHORD RECOGNITION ENGINE — SPECIFICATION v0.9
**Module 5 — Input Gateway của Arranger Engine — Trạng thái: Draft**

---

## 1. PHẠM VI & VỊ TRÍ TRONG HỆ THỐNG

Chord Engine ngồi giữa **nguồn input** (MIDI keyboard, on-screen pad, external controller) và **Arranger Engine**. Nhiệm vụ duy nhất: biến tập hợp note đang giữ → `Chord {root, type, bassNote, timestamp}` rồi đẩy vào Arranger Engine sớm nhất có thể.

```
MIDI Keyboard (left zone)  ─┐
On-screen Chord Pad        ─┼─→ Chord Engine → chordQueue → Arranger Engine
External MIDI controller   ─┘        ↑
                                 SplitPoint, Mode, HoldState
                                 (từ UI / Performance Preset)
```

**Chord Engine không phát âm thanh.** Note tay trái (dưới split point) đi vào Chord Engine; note tay phải (trên split point) đi thẳng sang Right Hand Voice Engine (phase 2) và không ảnh hưởng nhận diện hợp âm trừ khi `mode = fullKeyboard`.

**Yêu cầu latency:** từ Note On vật lý → `Chord` vào `chordQueue` ≤ **5ms**. Phần còn lại của ngân sách 20ms thuộc về audio buffer và MIDI output.

---

## 2. KIẾN TRÚC THREADING

```
CoreMIDI callback (realtime thread)
   ↓
InputRouter
   ├── note >= splitPoint → Right Hand queue (bypass)
   └── note < splitPoint  → ChordEngine.pushNote(note, vel, timestamp)
                                  ↓
                           cập nhật heldNotes (lock-free bitmask 128-bit)
                                  ↓
                           recognize() → Chord?
                                  ↓ (nếu chord mới khác chord hiện tại)
                           chordQueue.push(chord)
```

- `heldNotes` = lock-free bitmask 128-bit (2 × uint64) — set/clear bằng atomic ops, không allocate.
- `recognize()` chạy ngay trong CoreMIDI callback — phải **< 1µs**, dùng lookup table hoàn toàn.
- `chordQueue` = SPSC lock-free ring buffer, Arranger Engine drain trong audio callback.

---

## 3. CÁC MODE NHẬN DIỆN

### 3.1. Tổng quan

| Mode | Mô tả | Dùng khi |
|---|---|---|
| `fingered` | Nhận diện đầy đủ từ các note đang giữ | Pianist có kỹ thuật |
| `singleFinger` | Quy ước 1–3 ngón tắt, giống Yamaha | Người mới / chơi nhanh |
| `fullKeyboard` | Toàn bàn phím tham gia nhận diện | Phase 2 |

### 3.2. Mode `fingered`

Input: tất cả note đang giữ trong left-hand zone. Chạy recognition pipeline (§4). Note-count = 0 → emit `CHORD_NONE` (trừ khi holdMode bật).

### 3.3. Mode `singleFinger` — quy ước Yamaha

| Ngón bấm | Kết quả |
|---|---|
| 1 phím trắng (root) | `root Major` |
| 1 phím đen | Phím trắng gần nhất bên trái → `root Major` |
| Root + phím trắng liền kề bên trái | `root minor` (vd C + B → Cm) |
| Root + phím đen liền kề bên trái | `root 7` (vd C + B♭ → C7) |
| Root + 2 phím trắng liền kề bên trái | `root minor 7` (vd C + B + A → Cm7) |

Không "sáng tạo" quy ước khác — người dùng đã thuộc tay Yamaha.

### 3.4. Mode `fullKeyboard` (phase 2)

Toàn bàn phím tham gia; note cao nhất ngoài chord tone = right hand melody. Cần heuristic phân tách melody/chord — phase 2 cùng AI assist.

---

## 4. CHORD RECOGNITION PIPELINE (Fingered Mode)

### 4.1. Tổng quan

```
heldNotes (bitmask 128-bit)
   ↓ 1. extractPitchClasses()  → pitchClasses (12-bit) + lowestNote
   ↓ 2. detectBassNote()       → bassNote, rootCandidates
   ↓ 3. matchChordType()       → (root, type, score) best match
   ↓ 4. resolveSlash()         → nếu bassNote%12 ≠ root → slash chord
   ↓ 5. buildChord()           → Chord {root, type, bassNote, timestamp}
```

### 4.2. extractPitchClasses

```cpp
uint16_t pitchClasses = 0;
uint8_t  lowestNote   = 127;
for note in 0..127:
  if heldNotes.test(note):
    pitchClasses |= (1 << (note % 12));
    if note < lowestNote: lowestNote = note;
```

### 4.3. matchChordType — lookup table

**Precompute khi app start:**
```
chordLookup: HashMap<uint16_t pitchClassSet, List<(root, chordType)>>
```
34 chord types × 12 root × rotation = ~408 entry exact + inversions.

**Runtime:**
```
candidates = chordLookup[pitchClasses]   // exact match O(1)
if empty → bestPartialMatch(pitchClasses)
best = ưu tiên candidate có root = bassNote%12
       nếu không → highest score
```

### 4.4. Partial Match — bấm không hoàn hảo

Nhạc công bấm nhanh: có thể thiếu 1 note hoặc dư 1 note. Không được reject.

```
score = matched×2  -  extra×1  -  missing×3
// missing nặng hơn extra: thiếu chord tone tệ hơn bấm thêm tension
```

**Note count heuristics:**
- 1 note → root Major (không gọi match).
- 2 note → kiểm tra fifth (root+7) trước; nhiều ambiguous → Major.
- ≥ 3 note → full match.

Nếu nhiều candidate cùng score → ưu tiên chord đơn giản hơn (maj > 7 > maj7 > ...) để tránh over-recognition.

### 4.5. resolveSlash

```cpp
if (lowestNote % 12 != root):
  chord.bassNote = lowestNote;   // slash: C/E, Am/C...
  // root giữ nguyên
```

Arranger Engine nhận `bassNote` → bass track ưu tiên bassNote thay root (§6 Arranger Spec).


## 5. TẬP HỢP CHORD TYPE — MVP VÀ UASF

### 5.1. 14 loại MVP (nhận diện đầy đủ)

| Tên hiển thị | `chordType` UASF | Interval set (pitch-class relative to root) |
|---|---|---|
| Major | `maj` | {0, 4, 7} |
| minor | `min` | {0, 3, 7} |
| 7 (Dominant 7) | `7` | {0, 4, 7, 10} |
| Major 7 | `maj7` | {0, 4, 7, 11} |
| minor 7 | `min7` | {0, 3, 7, 10} |
| sus4 | `sus4` | {0, 5, 7} |
| dim | `dim` | {0, 3, 6} |
| aug | `aug` | {0, 4, 8} |
| 6 | `maj6` | {0, 4, 7, 9} |
| m6 | `min6` | {0, 3, 7, 9} |
| add9 | `maj` + {2} | {0, 2, 4, 7} — nhận diện riêng |
| 9 | `7` + {2} | {0, 2, 4, 7, 10} |
| m9 | `min7` + {2} | {0, 2, 3, 7, 10} |
| Slash chord | chordType bình thường + bassNote | xử lý bước 4.6 |

### 5.2. Bổ sung phase 2 (có trong UASF, chưa nhận diện ở MVP)

`dim7`, `min7b5`, `7sus4`, `7b5`, `7_9`, `7s11`, `7_13`, `7b9`, `7b13`, `sus2`, `minMaj7`, `maj7s11`, `maj9`, `maj7_9`, `min9`, `min7_11` — precompute sẵn trong lookup table nhưng UI phase 2 mới hiển thị.

---

## 6. BASS NOTE & SPLIT POINT

### 6.1. Split Point

```
splitPoint: uint8_t    // default = 60 (C4, Middle C)
// note < splitPoint  → left hand → Chord Engine
// note >= splitPoint → right hand → bypass
```

UI cho phép kéo split point từng semitone hoặc học bằng "hold lowest left + highest right" (split learn). Lưu trong Performance Preset.

### 6.2. Manual Bass Mode

Khi `manualBassMode = true`:
- Note thấp nhất tay trái (dưới `bassZoneTop`) = bass note — luôn dùng làm `bassNote` bất kể root.
- Các note còn lại tay trái → nhận diện chord type bình thường.
- Cho phép bấm C + E + G tay trái, nốt bass riêng (vd A dưới) → chord = C/A.

`bassZoneTop` mặc định = `splitPoint - 12` (1 octave dưới split). Có thể cấu hình.

---

## 7. HOLD MODE & CHORD MEMORY

### 7.1. Hold Mode

Khi `holdMode = true`:
- Release hết note tay trái → chord **không** emit CHORD_NONE, giữ nguyên chord hiện tại.
- Chord mới chỉ cập nhật khi có Note On mới.
- Hiển thị trên UI: icon lock + chord hiện tại nhấp nháy nhẹ.
- Dùng khi: nhạc công cần tay trái làm việc khác.

### 7.2. Chord Memory

`chordMemory`: circular buffer 16 chord gần nhất. Dùng cho:
- Hiển thị lịch sử hợp âm trên UI.
- Phase 2: AI suggest chord tiếp theo.
- Phase 2: "replay" pattern hợp âm.

### 7.3. Chord Latch

Mỗi lần nhả phím hoàn toàn rồi bấm lại chord cũ → giữ nguyên, không retrigger Arranger Engine (tránh glitch). So sánh `newChord == currentChord` trước khi emit — chỉ emit khi chord thực sự thay đổi. Threshold release ngắn: **80ms** (dưới đó = legato, không emit CHORD_NONE).

---

## 8. CHORD DISPLAY

```
root   = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"][root]
suffix = chordTypeSuffix[type]   // "m", "7", "maj7", "m7", "sus4", ...
slash  = bassNote%12 != root ? "/" + rootName[bassNote%12] : ""
display = root + suffix + slash
// ví dụ: "Am7", "C/E", "Gsus4"
```

Font lớn, màu nổi, luôn hiển thị trên Live Screen.

---

## 9. ON-SCREEN CHORD PAD

Dành cho người chơi không có MIDI keyboard.

**Layout MVP:** Grid root × type.
- 12 root (hàng ngang, C → B).
- 8 type phổ biến (maj, min, 7, maj7, min7, sus4, dim, aug) — hàng dọc.
- Tap root giữ → tap type → emit chord.
- Bấm 2 root đồng thời → slash chord thủ công.

Sau khi emit vào `chordQueue`, Arranger Engine không phân biệt nguồn.

---

## 10. `Chord` STRUCT (giao tiếp với Arranger Engine)

```cpp
struct Chord {
  uint8_t   root;          // 0-11, C=0
  ChordType type;          // enum 34 giá trị (UASF enum)
  uint8_t   bassNote;      // 0-127; = root×n nếu không slash
  uint64_t  hostTimestamp; // CoreMIDI/CoreAudio host time khi bấm
  InputSource source;      // midi_keyboard | onscreen_pad | external
};

static const Chord CHORD_NONE = { .root = 255 }; // sentinel
```

`hostTimestamp` dùng để Arranger Engine tính frame offset chính xác — bấm hợp âm trước beat 1 frame thì retrigger cũng chính xác tại frame đó, không bị trễ 1 buffer.

---

## 11. CONFIGURATION (lưu trong Performance Preset)

```json
{
  "chordEngine": {
    "mode":           "fingered",
    "splitPoint":     60,
    "holdMode":       false,
    "manualBassMode": false,
    "bassZoneTop":    48,
    "chordQuantize":  "off",
    "enharmonicPref": "sharp"
  }
}
```

---

## 12. KIỂM THỬ

### 12.1. Unit test — nhận diện

34 chord type × 12 root × 3 position (root, 1st inv, 2nd inv) = 1,224 test case cơ bản. Tất cả phải pass exact match.

```
test("C minor 7 root position"):
  input  = {60, 63, 67, 70}   // C Eb G Bb
  expect = Chord{root=0, type=min7, bassNote=60}

test("G/B slash chord"):
  input  = {47, 55, 59, 67}   // B G B G (B thấp nhất)
  expect = Chord{root=7, type=maj, bassNote=47}
```

### 12.2. Partial match test

```
test("Cmaj7 thiếu fifth"):
  input  = {60, 64, 71}            // C E B (thiếu G)
  expect = Chord{root=0, type=maj7}

test("Cmaj7 dư note tension"):
  input  = {60, 62, 64, 67, 71}   // C D E G B (thêm D)
  expect = Chord{root=0, type=maj7}   // không override thành maj9
```

### 12.3. Single Finger test

```
test("C7 single finger"):
  input  = {60, 58}   // C + Bb
  expect = Chord{root=0, type=7}

test("Cm single finger"):
  input  = {60, 59}   // C + B
  expect = Chord{root=0, type=min}
```

### 12.4. Latency test

Đo `pushNote()` → `chordQueue.push()` bằng `clock_gettime`. Target: **P99 < 100µs**.

### 12.5. Property / fuzz test

Random `heldNotes` bitmask → `recognize()` phải luôn trả về kết quả hợp lệ: không crash, root ∈ [0,11], bassNote ∈ [0,127].

---

## 13. GIỚI HẠN & OPEN QUESTIONS

**Giới hạn v1.0:**
- Polychord → phase 2.
- Enharmonic context-aware → phase 2.
- `fullKeyboard` mode → phase 2.
- `noteGenerator` retrigger → phase 2.

**Open questions (chốt Sprint 0–1):**
1. Quy ước Single Finger edge case của Yamaha (phím đen đơn lẻ, bassZone tắt) — xác minh trên PSR/Tyros thật.
2. Chord latch threshold 80ms — xác nhận qua beta.
3. On-screen pad: giữ root + tap type, hay 1 nút = tổ hợp? → hỏi nhạc công beta.
4. Aftertouch/pitch bend tay trái: bỏ qua ở Chord Engine, forward nguyên về MIDI Engine cho external module.

---

## 14. NEXT STEP

1. Implement lookup table generator offline bằng Python trước khi port C++ — 1 tuần.
2. Sprint 0 spike: CoreMIDI → nhận diện chord → log console (chưa cần Arranger Engine).
3. Tài liệu kế tiếp đề xuất:
   - **MIDI Engine Spec** — hoàn chỉnh vòng tròn cho Sprint 0.
   - **File Import Strategy** (Yamaha SFF1/SFF2 bit-level) — cần trước Sprint 1.
   - **Testing Strategy** — cần trước khi CI chạy.
