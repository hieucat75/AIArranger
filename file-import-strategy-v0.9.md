# FILE IMPORT STRATEGY — YAMAHA SFF1/SFF2
**Module 2 — Import Adapter — Trạng thái: Draft — Cần xác minh bit-level trong Sprint 0**

---

## 1. PHẠM VI

Tài liệu này mô tả: cấu trúc binary Yamaha Style File (.sty/.prs/.bcs/.sst), pipeline convert về UASF, mapping field-level SFF1/SFF2 → UASF, limitation list, và chiến lược test corpus.

**Nguyên tắc:** Import Adapter là offline process. Chạy trên background thread. Runtime Arranger Engine chỉ biết UASF — không bao giờ đọc .sty trực tiếp.

> ⚠️ **Về độ chính xác bit-level:** cấu trúc Yamaha Style File không được tài liệu hóa chính thức. Spec này tổng hợp từ JJazzLab/YamJJazz (open-source, đã reverse-engineer), vArranger documentation, và cộng đồng Yamaha Style. **Mọi offset/length/enum phải xác minh lại bằng test corpus Sprint 0** trước khi viết production parser.

---

## 2. TỔNG QUAN CẤU TRÚC FILE

Yamaha Style file là **SMF (Standard MIDI File) mở rộng** với:
- Header riêng (magic bytes format identifier).
- Track MIDI chứa tất cả section nối tiếp nhau, phân cách bằng **Marker meta-event**.
- Chunk `CASM` chứa chord transformation rules (nằm ngoài SMF chuẩn, append sau hoặc nhúng như track riêng).
- Chunk `OTS` (One Touch Settings) — bỏ qua v1.0.
- Chunk `MDB` (Music Database info) — bỏ qua.

### 2.1. Sơ đồ cấu trúc tổng thể

```
[Bytes 0–3]   Format Magic: "SFF1" hoặc "SFF2"
[Bytes 4–7]   SMF Header chunk "MThd" bắt đầu
              ↓
              Standard SMF Header (format=0, tracks=1, division=PPQ)
              ↓
"MTrk" — Track 0: toàn bộ MIDI data
  ├── Style Init Events (CC/PC setup cho từng channel)
  ├── [Marker: "SInt"]          ← Style Initialization block
  ├── [Marker: "Intro A"]
  │    └── MIDI events cho Intro 1
  ├── [Marker: "Intro B"]
  │    └── MIDI events cho Intro 2
  ├── [Marker: "Intro C"]       (nếu có)
  ├── [Marker: "Main A"]
  │    └── MIDI events cho Main A
  ├── [Marker: "Fill In AA"]
  ├── [Marker: "Fill In BB"]
  ├── [Marker: "Fill In CC"]    (nếu có)
  ├── [Marker: "Fill In DD"]    (nếu có)
  ├── [Marker: "Fill In AB"]    (cross-fill Main A→B)
  ├── ...
  ├── [Marker: "Main B/C/D"]
  ├── [Marker: "Break Fill"]
  ├── [Marker: "Ending A/B/C"]
  └── EOT (End of Track)

"CASM" chunk (Chord And Style Module):
  └── CSEG entries (per-track chord rules)
       └── Ctab (SFF1) hoặc Ctb2 (SFF2) per track

"OTS " chunk — bỏ qua v1.0
"MDB " chunk — bỏ qua
```

### 2.2. Extension và biến thể file

| Extension | Mô tả | Khác biệt |
|---|---|---|
| `.sty` | Style chuẩn | Format đầy đủ |
| `.prs` | Preset/Performance | Có thể chứa OTS, không chứa style data |
| `.bcs` | Backup style | Giống .sty, đôi khi nén |
| `.sst` | Style Set | Bundle nhiều style — cần unpack trước |

MVP parser: `.sty` trước, `.bcs` thứ hai (cùng structure), `.sst` phase 2 (cần unpack layer).

---

## 3. PARSE HEADER & FORMAT DETECTION

```cpp
bool detectFormat(FileReader& r, StyleFormat& outFmt):
  uint8_t magic[4];
  r.read(magic, 4);

  if magic == "SFF1":
    outFmt = SFF1;
  elif magic == "SFF2":
    outFmt = SFF2;
  else:
    // Một số file cũ không có magic, bắt đầu thẳng bằng "MThd"
    r.seek(0);
    if r.peekBytes(4) == "MThd":
      outFmt = SFF1_LEGACY;   // treat as SFF1, log warning
    else:
      return false;  // reject: unknown format
```

### 3.1. SMF Header Parse

Sau magic bytes (hoặc từ byte 0 với legacy):

```
"MThd"          4 bytes   — SMF header tag
length          4 bytes   — big-endian uint32, thường = 6
format          2 bytes   — big-endian uint16, phải = 0 (format 0)
numTracks       2 bytes   — phải = 1 (Yamaha style = single track)
division        2 bytes   — PPQ (ticks per quarter note), thường 48 hoặc 96 hoặc 480
                            bit 15 = 0 → ticks-per-quarter
```

Validate: format≠0 hoặc numTracks≠1 → reject với error `INVALID_SMF_HEADER`.

`division` → lưu vào `ppq`, dùng để normalize tick khi convert về UASF (UASF dùng 480 PPQ chuẩn hóa).

---

## 4. PARSE TRACK — SECTION EXTRACTION

Track bắt đầu bằng:
```
"MTrk"    4 bytes
length    4 bytes big-endian   — byte count của toàn bộ track data
```

Đọc events SMF chuẩn (variable-length delta time + event data):

### 4.1. Event types cần xử lý

| Event type | Xử lý |
|---|---|
| Note On (0x9n) | Giữ lại vào section hiện tại |
| Note Off (0x8n) | Giữ lại vào section hiện tại |
| Control Change (0xBn) | Giữ SInt CC vào soundMap/fx; giữ mid-section CC vào section MIDI |
| Program Change (0xCn) | Giữ SInt PC vào soundMap; bỏ mid-section PC (engine kiểm soát) |
| Pitch Bend (0xEn) | Giữ lại |
| Channel Pressure (0xDn) | Giữ lại |
| Meta 0x06 (Marker) | **Section boundary** — quan trọng nhất |
| Meta 0x51 (Tempo) | Lấy tempo đầu tiên → `music.tempoBpm`; bỏ các tempo sau |
| Meta 0x58 (Time Sig) | Lấy → `music.timeSignature`; validate consistent |
| Meta 0x2F (EOT) | Kết thúc track |
| SysEx (0xF0/0xF7) | SInt SysEx XG/GS → parse vào soundMap; còn lại bỏ qua |

### 4.2. Marker text → Section mapping

```
Marker text          SectionId           type      ordinal
─────────────────────────────────────────────────────────
"SInt"               —                   init      —      ← không phải section, là setup
"Intro A"            intro_1             intro     1
"Intro B"            intro_2             intro     2
"Intro C"            intro_3             intro     3
"Main A"             main_a              main      1
"Main B"             main_b              main      2
"Main C"             main_c              main      3
"Main D"             main_d              main      4
"Fill In AA"         fill_a_self         fill      —      ← fill của Main A, quay về A
"Fill In BB"         fill_b_self         fill      —
"Fill In CC"         fill_c_self         fill      —
"Fill In DD"         fill_d_self         fill      —
"Fill In AB"         fill_a_to_b         fill      —      ← cross-fill A→B
"Fill In BA"         fill_b_to_a         fill      —
"Fill In BC"         fill_b_to_c         fill      —
(và các combination khác nếu có)
"Break Fill"         break               break     —
"Ending A"           ending_1            ending    1
"Ending B"           ending_2            ending    2
"Ending C"           ending_3            ending    3
```

Marker text matching: case-insensitive, trim whitespace. Một số style dùng "Main AA" thay "Main A" — normalize.

**Cross-fill (Fill In AB):** v1.0 convert về `fill_a_self` và log `FILL_CROSS_DEGRADED`. Engine xử lý transition theo logic section switch, không phụ thuộc fill type.

### 4.3. SInt — Style Initialization Parse

SInt là section đặc biệt: không chứa note, chỉ chứa CC/PC setup cho từng channel. Tất cả event trong SInt → parse vào `soundMap` và `fx` hints của UASF:

```
Per channel (duyệt CC/PC events trong SInt section):
  CC#0  (Bank MSB)       → soundMap[ch].bankMsb
  CC#32 (Bank LSB)       → soundMap[ch].bankLsb
  PC                     → soundMap[ch].program
  CC#7  (Volume)         → tracks[ch].defaultVolume
  CC#10 (Pan)            → tracks[ch].defaultPan
  CC#91 (Reverb)         → fx.reverbSend[trackId]
  CC#93 (Chorus)         → fx.chorusSend[trackId]
  CC#11 (Expression)     → bỏ qua (engine reset về 127)
  SysEx XG voice setup   → parse bank/voice nếu XG format, bổ sung soundMap
```

Channel → trackId mapping (Yamaha convention, SFF1):
```
Ch 10 (index 9)  → drums (role: drums, isRhythm: true)
Ch 11 (index 10) → bass
Ch 12 (index 11) → chord_1  (role: chord)
Ch 13 (index 12) → chord_2
Ch 14 (index 13) → chord_3  (role: pad)
Ch 15 (index 14) → phrase_1 (role: phrase)
Ch 16 (index 15) → phrase_2
Ch  9 (index 8)  → percussion (isRhythm: true) — nếu có
```

SFF2 có thể dùng nhiều channel hơn — detect bằng cách scan event, không hardcode.

---

## 5. CASM CHUNK PARSE

CASM là chunk nằm **sau** MTrk data (hoặc nhúng trong một MTrk track phụ ở một số variant).

### 5.1. Tìm CASM

```cpp
// Sau khi parse hết MTrk:
while not EOF:
  char tag[4] = r.read(4)
  uint32_t len = r.readBigEndian32()
  if tag == "CASM":
    parseCasm(r.slice(len))
  elif tag == "OTS ":
    skipChunk(len)   // + log OTS_SKIPPED
  elif tag == "MDB ":
    skipChunk(len)
  else:
    skipChunk(len)   // unknown chunk — skip gracefully
```

### 5.2. Cấu trúc bên trong CASM

```
CASM data:
  [CSEG entries, liên tiếp nhau đến hết length]

Mỗi CSEG entry:
  "CSEG"              4 bytes  — tag
  length              2 bytes  — big-endian uint16, byte count của CSEG này
  [Sdec data]
  [Ctab/Ctb2 entries]
```

### 5.3. Sdec (Style Declaration)

```
Sdec header:
  "Sdec"        4 bytes
  length        2 bytes
  [Section name string, null-terminated hoặc fixed length theo length]
```

Sdec liên kết CSEG này với section nào. Section name khớp với Marker text đã parse ở §4.2.

### 5.4. Ctab (SFF1) — Field Layout

Đây là trái tim của CASM. Mỗi Ctab = rules cho 1 track trong section này.

```
Ctab header:
  "Ctab"         4 bytes
  length         2 bytes   — byte count của Ctab này

Ctab data (theo thứ tự byte):
  Offset  Len  Field
  ──────────────────────────────────────────────────────
   0       1   srcChannel        source MIDI channel (1-based)
   1       1   dstChannel        destination MIDI channel (1-based)
   2      32   name              ASCII, null-padded (track name, vd "Melody")
  34       1   noteLowLimit      lowest MIDI note allowed (0-127)
  35       1   noteHighLimit     highest MIDI note allowed
  36       1   sourceMusicType   chord type của style gốc (enum, xem §5.5)
  37       1   sourceRoot        root của style gốc (0=C, 1=C#, ...)
  38       2   chordMuteBitmask  16-bit, mỗi bit = 1 chord type bị mute
                                  (thứ tự bit = thứ tự chord type enum Yamaha)
  40       2   noteMuteBitmask   12-bit (low 12 của 16-bit), mỗi bit = 1 pitch class
  42       1   ntr               Note Transposition Rule (enum, xem §5.6)
  43       1   ntt               Note Transposition Table (enum, xem §5.7)
  44       1   highKey           breakpoint key (0-127)
  45       1   noteTransp        note transpose offset (signed, hiếm dùng)
  46       1   rtr               Retrigger Rule (enum, xem §5.8)
  [end, tổng ~47 bytes — có thể có padding tới length]
```

> ⚠️ **Offset chính xác CẦN xác minh** bằng JJazzLab source + test corpus. Thứ tự và offset trên là best-effort từ reverse-engineering; có thể có padding byte hoặc field bổ sung tùy firmware máy.

### 5.5. Source Music Type Enum (Yamaha → UASF chordType)

```
Yamaha value → UASF chordType
0x00 → maj
0x01 → maj6
0x02 → maj7
0x03 → maj7s11
0x04 → maj9
0x05 → maj7_9
0x06 → maj6_9
0x07 → aug
0x08 → min
0x09 → min6
0x0A → min7
0x0B → min7b5
0x0C → min9
0x0D → min7_9
0x0E → min7_11
0x0F → minMaj7
0x10 → minMaj7_9
0x11 → dim
0x12 → dim7
0x13 → 7
0x14 → 7sus4
0x15 → 7b5
0x16 → 7_9
0x17 → 7s11
0x18 → 7_13
0x19 → 7b9
0x1A → 7b13
0x1B → 7s9
0x1C → maj7aug
0x1D → 7aug
0x1E → sus4
0x1F → sus2
0x20 → 1plus5
0x21 → 1plus8
```

34 giá trị — khớp enum UASF. `chordMuteBitmask` dùng thứ tự này (bit 0 = maj, bit 1 = maj6, ...).

### 5.6. NTR Enum

```
0x00 → rootTransposition
0x01 → rootFixed
0x02 → guitar          (SFF2; v1.0 fallback rootTransposition)
```

### 5.7. NTT Enum

```
SFF1:
0x00 → bypass
0x01 → melody
0x02 → chord
0x03 → bass
0x04 → melodicMinor
0x05 → harmonicMinor

SFF2 bổ sung:
0x06 → naturalMinor
0x07 → dorian
0x08 → melodicMinorB5
0x09 → harmonicMinorB5
0x0A → naturalMinorB5
0x0B → dorianB5
0x0C → chordOnly
```

### 5.8. RTR (Retrigger) Enum

```
0x00 → stop
0x01 → pitchShift
0x02 → pitchShiftToRoot
0x03 → retrigger
0x04 → retriggerToRoot
0x05 → noteGenerator     (v1.0 fallback pitchShift)
```

---

## 6. CTB2 (SFF2) — KHÁC BIỆT SO VỚI CTAB

SFF2 thay Ctab bằng Ctb2. Điểm khác biệt chính:

```
"Ctb2"         4 bytes tag (thay "Ctab")
length         2 bytes

Ctb2 data:
  [Giống Ctab cho đến hết các field cơ bản]

  Bổ sung SFF2:
  numRanges      1 byte    — số vùng (1, 2 hoặc 3)
  
  Với mỗi range (low/mid/high):
    rangeHigh    1 byte    — note cao nhất của vùng này
    ntr          1 byte    — NTR riêng cho vùng này
    ntt          1 byte    — NTT riêng (dùng enum SFF2 mở rộng §5.7)

  guitarVoicingData:        — chỉ có khi ntr = guitar
    [dữ liệu voicing guitar, format phức tạp]
    v1.0: skip toàn bộ, log SFF2_GUITAR_NTR_DEGRADED
```

Mapping Ctb2 → UASF `noteRanges[]`:
- `numRanges = 1`: 1 entry, low=0 high=127.
- `numRanges = 2`: entry[0] low=0 high=range[0].rangeHigh; entry[1] low=range[0].rangeHigh+1 high=127.
- `numRanges = 3`: 3 entries tương tự.

Đây là lý do UASF schema dùng `noteRanges[]` mảng từ v1.0 — không cần migration khi bật SFF2 support.

---

## 7. DRUM MAP EXTRACTION

Drum track (channel 10, isRhythm=true) không qua NTR/NTT, nhưng cần map note numbers Yamaha → GM hoặc ghi nhận override.

```
Yamaha style drum notes nằm trong dải GM chuẩn (27–87) phần lớn.
Note ngoài dải GM → thêm vào drumMap.overrides:
  { "srcNote": <yamaha_note>, "dstNote": <gm_note>, "note": "<name>" }
```

Bảng mapping Yamaha note → GM note là constant được compile vào adapter (không phải user data). Cần xây dựng bảng này từ Yamaha drum kit documentation và test thực tế.

---

## 8. IMPORT PIPELINE

```
Input: .sty file (raw bytes)
       ↓
Step 1: detectFormat()           → SFF1 / SFF2 / Legacy
       ↓
Step 2: parseSMFHeader()         → ppq, validate
       ↓
Step 3: parseTrack()             → sections[], sintEvents[], tempoMap
       ↓
Step 4: parseSInt()              → soundMap[], fx hints, tracks[]
       ↓
Step 5: parseCasm()              → ctabMap: sectionId → [trackRules]
       ↓
Step 6: buildUASF()
         ├── manifest.meta       ← từ filename + user input
         ├── manifest.music      ← tempo, timeSig, ppq=480
         ├── manifest.tracks     ← từ SInt channel mapping
         ├── manifest.soundMap   ← từ SInt PC/Bank events
         ├── manifest.drumMap    ← từ drum note analysis
         ├── manifest.fx         ← từ SInt CC91/93
         ├── manifest.sections   ← từ section list + ctabMap
         └── manifest.import     ← sourceFormat, limitations log
       ↓
Step 7: writeSectionMidFiles()   → sections/*.mid (resample ticks → 480 PPQ)
       ↓
Step 8: validate()               → V01–V07 (theo UASF Spec §9)
       ↓
Step 9: packageZip()             → mystyle.uas
       ↓
Output: .uas file + ImportResult {success, warnings, limitations}
```

### 8.1. Tick Normalization

Nếu source PPQ ≠ 480:
```
newTick = round(srcTick × 480.0 / srcPpq)
```
Áp cho mọi event trong section MIDI. Rounding error < 0.1ms ở 480 PPQ — chấp nhận được.

### 8.2. Tempo Extraction

Lấy tempo event **đầu tiên** trong track (thường ở tick 0, trong SInt). Các tempo change mid-section → strip (engine kiểm soát tempo). Log nếu có multiple tempo events: `MULTI_TEMPO_STRIPPED`.

---

## 9. ERROR HANDLING & USER FEEDBACK

Mọi lỗi và cảnh báo đều sinh `ImportResult` với danh sách rõ ràng. UI hiển thị sau import:

```swift
struct ImportResult {
  var success: Bool
  var outputPath: URL?
  var errors:   [ImportIssue]   // chặn import (V01–V07)
  var warnings: [ImportIssue]   // import được, cần chú ý
  var infos:    [ImportIssue]   // thông tin về limitation đã xử lý
}

struct ImportIssue {
  var code: String         // "SFF2_GUITAR_NTR_DEGRADED"
  var severity: Severity   // error | warning | info
  var message: String      // tiếng Việt/Anh theo locale
  var trackId: String?     // nếu liên quan đến track cụ thể
  var sectionId: String?
}
```

**UI import flow:**
1. User drop file → progress indicator chạy nền.
2. Thành công: style xuất hiện trong library, badge "1 cảnh báo" nếu có warning.
3. Lỗi chặn: dialog mô tả lỗi, không import, giữ file gốc.
4. User tap "Xem chi tiết" → danh sách đầy đủ ImportResult.

---

## 10. CHIẾN LƯỢC TEST CORPUS

### 10.1. Thu thập

Target: **500+ style file** đa dạng:
- Đa đời máy: PSR-E, PSR-S series, Tyros 1–5, Genos, Pa (để detect quirk per-model).
- Đa genre: pop, rock, ballad, waltz, bossa, folk, latin, jazz.
- Đa thời gian: style cũ (SFF1 thuần) + style mới (SFF2).
- Đa nguồn: style built-in + style download từ forum Yamaha cộng đồng (user-owned).

Cách thu thập hợp pháp: nhạc công beta team export từ máy họ sở hữu. Không download style không rõ nguồn gốc.

### 10.2. Test tự động

```
Với mỗi file trong corpus:
  1. Import → kiểm tra không crash, trả về ImportResult
  2. Validate UASF output (V01–V07)
  3. Render Main A 8 bar → output MIDI
  4. So sánh note count, pitch distribution với expected (tự build expected từ lần đầu)
  5. Flag regression nếu khác

Metric đo:
  - Import success rate (target: ≥95% SFF1)
  - Warning rate (phân loại theo code)
  - Section parse rate (% section không bị miss)
  - Tick accuracy (drift < 1 tick sau normalize)
```

### 10.3. A/B Test nhạc tính

Phát cùng style + cùng chuỗi hợp âm qua:
- App (UASF + Arranger Engine)
- Yamaha keyboard tham chiếu (thu audio hoặc MIDI)

Nhạc công beta nghe và chấm điểm 1–5 cho: (a) đúng nhịp, (b) đúng hòa âm, (c) cảm giác mượt khi đổi hợp âm.

### 10.4. Regression Test CI

Mỗi commit → chạy tự động trên subset 50 style (nhanh nhất trong corpus). Full 500 style chạy nightly.

---

## 11. LIMITATION LIST CHÍNH THỨC (v1.0)

| Code | Mô tả | Severity |
|---|---|---|
| `OTS_SKIPPED` | One Touch Settings không import | info |
| `SFF2_GUITAR_NTR_DEGRADED` | Guitar voicing NTR → Root Transposition | warning |
| `SFF2_MEGAVOICE` | Mega Voice articulation bị bỏ qua | warning |
| `FILL_CROSS_DEGRADED` | Cross-fill (Fill AB) convert về self-fill | info |
| `MULTI_TEMPO_STRIPPED` | Nhiều tempo event, chỉ giữ đầu tiên | info |
| `DRUM_NOTE_UNMAPPED` | Drum note ngoài bảng map → giữ nguyên | warning |
| `SECTION_LENGTH_ADJUSTED` | Độ dài section đã pad/trim để khớp beat | info |
| `UNKNOWN_CHORD_TYPE` | Giá trị chord type lạ → map về `maj` | warning |
| `UNKNOWN_NTT` | Giá trị NTT không nhận ra → `bypass` | warning |
| `TIMESIG_INCONSISTENT` | Time signature đổi giữa section → lấy đầu tiên | warning |
| `SYSEX_XG_PARTIAL` | SysEx XG parse một phần, soundMap có thể không đầy đủ | info |
| `INVALID_SMF_HEADER` | SMF header lỗi | **error (chặn)** |
| `NO_MAIN_SECTION` | Không tìm thấy section Main nào | **error (chặn)** |
| `CASM_MISSING` | Không có CASM chunk → import MIDI, không có chord rules | warning |
| `CASM_PARSE_ERROR` | CASM parse lỗi → dùng default rules (bypass NTT) | warning |

---

## 12. OPEN QUESTIONS (Sprint 0 — ưu tiên cao nhất)

1. **Offset Ctab chính xác** — lấy JJazzLab `YamJJazz` source code (LGPL, không copy, chỉ tham khảo algorithm) + chạy test corpus + hex dump 10 file. Đây là task đầu tiên của Sprint 0.
2. **CASM position:** luôn sau MTrk, hay có file nào nhúng CASM trong track riêng? Scan corpus để biết.
3. **Sdec encoding:** section name trong Sdec là ASCII thuần hay có thể Shift-JIS (style tiếng Nhật)? Handle cả hai.
4. **SFF2 numRanges encoding:** nằm ở đâu chính xác trong Ctb2, trước hay sau các field cơ bản? Xác minh.
5. **Style .sst unpack:** cần cho phase 2, research format sớm ngay Sprint 1 để không bị surprise.
