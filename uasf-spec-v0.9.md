# UNIVERSAL ARRANGER STYLE FORMAT (UASF) — SPECIFICATION v0.9
**File extension: `.uas` — Tài liệu kỹ thuật nội bộ — Trạng thái: Draft chờ validate bằng test corpus (Sprint 0)**

---

## 1. MỤC ĐÍCH & PHẠM VI

UASF là định dạng style nội bộ duy nhất mà Arranger Engine hiểu. Mọi format hãng (Yamaha, sau này Korg/Roland/Ketron) được convert về UASF qua Import Adapter. Runtime **không bao giờ** đọc trực tiếp file hãng.

Spec này định nghĩa: cấu trúc container, schema `manifest.json`, quy ước section MIDI, mô hình chord transformation, mapping Yamaha SFF1/SFF2 → UASF, validation rules, versioning.

**Ngoài phạm vi v0.9:** audio-loop style (Ketron), OTS (One Touch Setting) chi tiết, style editing/recording.

---

## 2. NGUYÊN TẮC THIẾT KẾ

1. **Superset có kiểm soát:** UASF phải biểu diễn được 100% SFF1 và phần MIDI-chuẩn của SFF2, nhưng không sao chép mù cấu trúc Yamaha — mô hình hóa lại theo khái niệm trung lập.
2. **Metadata người đọc được, event máy đọc nhanh:** JSON cho rule/metadata, SMF cho MIDI events.
3. **Mọi field có default:** manifest thiếu field không bắt buộc → engine dùng default, không fail. Fail chỉ xảy ra ở validation lúc import, không ở runtime.
4. **Forward-compatible:** engine cũ gặp field lạ → bỏ qua (ignore-unknown). Engine mới gặp file cũ → migration tự động.
5. **Một file = một style,** tự chứa (trừ sample pack tham chiếu ngoài, có dependency khai báo).

---

## 3. CONTAINER

`.uas` = ZIP (deflate, không mã hóa) với cấu trúc:

```
mystyle.uas
├── manifest.json          # BẮT BUỘC — toàn bộ metadata + rules
├── sections/              # BẮT BUỘC — mỗi section 1 file SMF
│   ├── intro_1.mid
│   ├── main_a.mid
│   ├── fill_a.mid
│   ├── ...
│   └── ending_1.mid
├── samples/               # TÙY CHỌN (phase 2) — sample nhúng
│   └── *.wav | *.flac
├── preview.m4a            # TÙY CHỌN — audio demo cho browser/marketplace
└── source.bin             # TÙY CHỌN — file gốc giữ nguyên để re-import
                           #   khi adapter được nâng cấp (user bật/tắt)
```

Quy tắc: tên file trong zip là ASCII lowercase + underscore; `manifest.json` phải là entry đầu tiên (cho phép đọc metadata nhanh không giải nén toàn bộ — quan trọng khi scan library hàng nghìn style).

---

## 4. SCHEMA `manifest.json`

### 4.1. Cấp cao nhất

```json
{
  "uasf": {
    "formatVersion": "1.0",
    "minEngineVersion": "1.0"
  },
  "meta": { ... },          // 4.2
  "pack": { ... },          // 4.3 — license/ownership, có từ ngày đầu
  "music": { ... },         // 4.4 — tempo, time signature
  "tracks": [ ... ],        // 4.5 — khai báo track toàn cục
  "sections": [ ... ],      // 4.6 — danh sách section + rules per track
  "soundMap": [ ... ],      // 4.7
  "drumMap": { ... },       // 4.8
  "fx": { ... },            // 4.9 — hints, không bắt buộc
  "import": { ... }         // 4.10 — provenance & limitation log
}
```

### 4.2. `meta`

```json
"meta": {
  "id": "uuid-v4",
  "name": "80s Pop Shuffle",
  "genre": ["pop", "shuffle"],
  "tags": ["wedding", "retro"],
  "author": "…",
  "description": "…",
  "createdAt": "2026-06-20T10:00:00Z",
  "modifiedAt": "…"
}
```

### 4.3. `pack` (chuẩn bị cho marketplace — bắt buộc có từ v1.0)

```json
"pack": {
  "packId": null,
  "license": "user-imported | cc0 | commercial | custom",
  "licenseText": null,
  "sampleOwnership": "none | embedded-owned | external-ref",
  "dependencies": [
    { "type": "soundpack", "id": "…", "minVersion": "1.0" }
  ],
  "version": "1.0.0",
  "compatibility": { "minEngine": "1.0" }
}
```

### 4.4. `music`

```json
"music": {
  "tempoBpm": 112.0,
  "timeSignature": { "numerator": 4, "denominator": 4 },
  "ppq": 480
}
```

`ppq` áp dụng cho mọi SMF trong `sections/` (adapter resample tick nếu nguồn khác). Tempo/time-signature là giá trị mặc định của style; tempo event trong section MIDI bị strip (engine kiểm soát tempo).

### 4.5. `tracks` — khai báo toàn cục

Tối đa 16 track. Mỗi track ánh xạ 1 MIDI channel trong các SMF section.

```json
"tracks": [
  {
    "trackId": "rhythm_main",
    "name": "Drums",
    "role": "drums | percussion | bass | chord | pad | phrase | other",
    "midiChannel": 9,
    "isRhythm": true,
    "defaultVolume": 100,
    "defaultPan": 64
  }
]
```

- `role` là semantic hint cho UI (mute group "Acc/Bass/Drum") và AI layer sau này — engine không phụ thuộc.
- `isRhythm: true` → track không qua chord transformation (tương đương kênh drum/perc Yamaha).
- Quy ước channel khi convert từ Yamaha: giữ nguyên destination channel gốc (drum=9, bass=11, v.v. theo style nguồn) để MIDI output ra module ngoài "ăn" ngay.

### 4.6. `sections` — phần quan trọng nhất

```json
"sections": [
  {
    "sectionId": "main_a",
    "type": "intro | main | fill | break | ending",
    "label": "Main A",
    "ordinal": 1,
    "file": "sections/main_a.mid",
    "lengthBeats": 16,
    "loop": true,
    "fillTarget": null,
    "trackRules": [ { ...TrackRule... } ]
  }
]
```

- `type=fill` có `fillTarget`: `"self" | "next"` (fill quay về chính main hay chuyển main — mô hình hóa Fill AA vs Fill AB của Yamaha; UASF tổng quát hóa: fill thuộc về main nguồn, target do arranger quyết theo lệnh user, nên đa số trường hợp `fillTarget: "self"` và transition do engine xử lý).
- `loop`: main=true; intro/fill/break/ending=false.
- `lengthBeats` do adapter tính từ SMF, validation đối chiếu.

**TrackRule — mô hình chord transformation (lõi của UASF):**

```json
{
  "trackId": "bass",
  "enabled": true,
  "sourceChord": { "root": 0, "type": "maj7" },
  "noteRanges": [
    {
      "lowKey": 0,
      "highKey": 127,
      "ntr": "rootTransposition | rootFixed | guitar",
      "ntt": "bypass | melody | chord | bass | melodicMinor |
              melodicMinorB5 | harmonicMinor | harmonicMinorB5 |
              naturalMinor | naturalMinorB5 | dorian | dorianB5 |
              chordOnly",
      "highKey": 127
    }
  ],
  "retrigger": "stop | pitchShift | pitchShiftToRoot |
                retrigger | retriggerToRoot | noteGenerator",
  "noteLowLimit": 0,
  "noteHighLimit": 127,
  "noteMute": [],
  "chordMute": [],
  "breakpointKey": 72
}
```

Giải thích các field then chốt:
- `sourceChord`: hợp âm gốc mà phần MIDI được thu (Yamaha mặc định CMaj7). Engine transform từ source chord → hợp âm user bấm.
- `noteRanges[]`: SFF1 chỉ có 1 range (toàn dải); SFF2 Ctb2 có tới 3 range (low/mid/high) với NTR/NTT riêng. **Schema dùng mảng từ v1.0** để SFF2 không cần migration — quyết định đã chốt ở tài liệu chiến lược.
- `ntr` (Note Transposition Rule): note theo root (`rootTransposition`) hay giữ cao độ và chỉ remap vào thang (`rootFixed`). `guitar` dành cho SFF2 guitar voicing — v1.0 engine map về `rootTransposition` + log limitation.
- `ntt` (Note Transposition Table): bảng quy đổi note khi đổi loại hợp âm. Tập giá trị là hợp nhất SFF1 ∪ SFF2 (đuôi `B5` = biến thể "với ♭5" của SFF2).
- `retrigger`: hành vi note đang vang khi đổi hợp âm — quyết định "cảm giác mượt" của arranger.
- `noteMute`: mảng pitch-class bị tắt (0–11). `chordMute`: mảng chord type mà track im lặng (vd track phrase tắt ở sus4).
- `breakpointKey` (SFF: "high key"): ranh giới quyết định octave khi transpose root để giai điệu không nhảy quãng vô lý.

### 4.7. `soundMap`

```json
"soundMap": [
  {
    "trackId": "bass",
    "program": 33,
    "bankMsb": 0,
    "bankLsb": 0,
    "gm2Fallback": 33,
    "internalPreset": "bass/fingered_standard"
  }
]
```

Ba tầng: (1) PC/Bank gốc — gửi nguyên ra external module; (2) `gm2Fallback` — khi module không nhận bank Yamaha; (3) `internalPreset` — preset của internal sound engine, adapter điền bằng bảng tra Yamaha-voice → internal-preset (deliverable riêng của Import Adapter).

### 4.8. `drumMap`

```json
"drumMap": {
  "base": "gm",
  "overrides": [ { "srcNote": 22, "dstNote": 42, "note": "HH closed alt" } ]
}
```

V1.0: chỉ hỗ trợ `base: "gm"` + overrides cho các note Yamaha ngoài GM (kit chuẩn Yamaha có các slot ngoài 27–87). Adapter log note không map được vào `import.limitations`.

### 4.9. `fx` (hints — engine được phép bỏ qua hoàn toàn ở v1.0)

```json
"fx": {
  "reverbSend": { "bass": 10, "phrase_1": 40 },
  "chorusSend": { "pad": 30 }
}
```

Lấy từ CC91/CC93 trong SInt của style gốc. Đây là **hint**, không phải FX graph — FX routing thật thuộc spec Sound Engine.

### 4.10. `import` — provenance

```json
"import": {
  "sourceFormat": "yamaha-sff1 | yamaha-sff2 | korg | …",
  "sourceFile": "CoolShuffle.S730.sty",
  "adapterVersion": "1.0.3",
  "importedAt": "…",
  "limitations": [
    { "code": "SFF2_GE_SKIPPED", "severity": "warn",
      "message": "Guitar Edition voicing trên track Guitar được map về Root Transposition." }
  ],
  "sourceHash": "sha256:…"
}
```

`limitations` hiển thị cho user ngay sau import (yêu cầu "show import error clearly" trong brief) và là dữ liệu vàng để đo chất lượng adapter trên test corpus.

---

## 5. QUY ƯỚC SECTION MIDI (`sections/*.mid`)

1. **SMF Format 0**, PPQ = `music.ppq` (chuẩn hóa 480).
2. Tick 0 = đầu section. Section dài đúng `lengthBeats × ppq` tick; adapter pad/trim và log nếu nguồn lệch.
3. **Được giữ:** Note On/Off, CC (1, 7, 10, 11, 64, 71–74, 91, 93), Pitch Bend, Program Change/Bank nếu style đổi voice giữa chừng (hiếm), Channel Pressure.
4. **Bị strip (engine kiểm soát):** tempo events, time signature, markers SFF, SysEx (trừ SysEx voice-setup được chuyển thành `soundMap`), CASM dĩ nhiên không nằm ở đây.
5. Note trong file là **note gốc theo `sourceChord`** — chưa transform. Mọi transformation xảy ra realtime trong Arranger Engine theo `trackRules`.
6. Mỗi track dùng đúng `midiChannel` khai báo trong `tracks`.

**Lý do tách mỗi section một file SMF** (thay vì 1 file + marker): load section độc lập (preload next section trong setlist), diff/debug từng section, và đơn vị cache tự nhiên cho engine.

---

## 6. MAPPING TABLE: YAMAHA → UASF

### 6.1. Cấu trúc file

| Yamaha | UASF |
|---|---|
| SMF section sau marker `SFF1`/`SFF2` | tách thành `sections/*.mid` theo marker section |
| Marker `SInt` (style init: PC/Bank/CC setup) | `soundMap` + `fx` hints + `tracks` defaults |
| Marker `Intro A/B/C` | section `type:intro, ordinal:1/2/3` |
| Marker `Main A–D` | `type:main, ordinal:1–4` |
| Marker `Fill In AA/BB/CC/DD` | `type:fill` gắn main tương ứng, `fillTarget:"self"` |
| Marker `Fill In BA` v.v. (chuyển main) | v1.0: map về fill của main đích + limitation log (engine tự xử lý transition) |
| Marker `Ending A/B/C` | `type:ending, ordinal:1/2/3` |
| Chunk `CASM › CSEG › Sdec` | liên kết section ↔ bộ Ctab |
| Chunk `OTS` | **bỏ qua v1.0** + limitation `OTS_SKIPPED` (phase 2: convert thành Performance preset) |
| Chunk `MDB`, `MH` | bỏ qua (metadata Music Finder — không ảnh hưởng playback) |
| Tempo/TimeSig trong SMF header | `music.tempoBpm`, `music.timeSignature` |

### 6.2. Ctab (SFF1) → TrackRule

| Ctab field | UASF | Ghi chú |
|---|---|---|
| Source channel | chọn track events trong SMF | |
| Destination channel | `tracks[].midiChannel` | |
| Name | `tracks[].name` | |
| Editable flag | bỏ qua (UASF luôn editable) | |
| Note mute bitmask (12 bit) | `noteMute[]` pitch-class | |
| Chord mute bitmask (36 chord types) | `chordMute[]` | tên chord type theo enum UASF |
| Source chord root + type | `sourceChord` | mặc định CMaj7 |
| NTR (2 giá trị) | `ntr: rootTransposition / rootFixed` | 1 `noteRanges` entry duy nhất, low=0 high=127 |
| NTT (6 giá trị SFF1) | `ntt: bypass/melody/chord/bass/melodicMinor/harmonicMinor` | |
| High key | `breakpointKey` | |
| Note low/high limit | `noteLowLimit/HighLimit` | |
| RTR (6 giá trị) | `retrigger` | mapping 1:1 theo enum mục 4.6 |

### 6.3. Ctb2 (SFF2) → TrackRule

| Ctb2 | UASF | Ghi chú |
|---|---|---|
| 3 vùng low/mid/high với NTR/NTT riêng | `noteRanges[]` 1–3 entries | schema đã sẵn, **đây là lý do dùng mảng từ v1.0** |
| NTT mở rộng (natural minor, dorian, biến thể ♭5) | enum `ntt` đầy đủ mục 4.6 | |
| NTR `guitar` + guitar voicing data | v1.0: `ntr:rootTransposition` + limitation `SFF2_GUITAR_NTR_DEGRADED` | phase 2 implement đúng |
| Các extension khác (sẽ phát hiện trong corpus) | limitation log | nguyên tắc: **không bao giờ fail im lặng** |

> ⚠️ **Ghi chú độ tin cậy:** mã hóa bit-level chính xác của CASM/Ctab/Ctb2 (offset, độ dài bitmask, thứ tự enum) phải được xác minh trong Sprint 0 bằng (a) đối chiếu mã nguồn JJazzLab YamJJazz, (b) chạy trên test corpus và so sánh với playback tham chiếu. Spec này chốt **mô hình UASF**; bảng mapping ở mức semantic và sẽ kèm phụ lục bit-level sau spike Sprint 0.

---

## 7. VÍ DỤ `manifest.json` HOÀN CHỈNH (rút gọn 2 track, 2 section)

```json
{
  "uasf": { "formatVersion": "1.0", "minEngineVersion": "1.0" },
  "meta": {
    "id": "9f1c2e8a-7b4d-4f3a-9e21-0c5d6a7b8c9d",
    "name": "Cool Shuffle",
    "genre": ["pop"], "tags": [], "author": null,
    "createdAt": "2026-06-20T10:00:00Z", "modifiedAt": "2026-06-20T10:00:00Z"
  },
  "pack": {
    "packId": null, "license": "user-imported",
    "sampleOwnership": "none", "dependencies": [],
    "version": "1.0.0", "compatibility": { "minEngine": "1.0" }
  },
  "music": {
    "tempoBpm": 112.0,
    "timeSignature": { "numerator": 4, "denominator": 4 },
    "ppq": 480
  },
  "tracks": [
    { "trackId": "drums", "name": "Drums", "role": "drums",
      "midiChannel": 9, "isRhythm": true,
      "defaultVolume": 110, "defaultPan": 64 },
    { "trackId": "bass", "name": "Bass", "role": "bass",
      "midiChannel": 11, "isRhythm": false,
      "defaultVolume": 100, "defaultPan": 64 }
  ],
  "sections": [
    {
      "sectionId": "main_a", "type": "main", "label": "Main A",
      "ordinal": 1, "file": "sections/main_a.mid",
      "lengthBeats": 16, "loop": true, "fillTarget": null,
      "trackRules": [
        { "trackId": "drums", "enabled": true },
        { "trackId": "bass", "enabled": true,
          "sourceChord": { "root": 0, "type": "maj7" },
          "noteRanges": [
            { "lowKey": 0, "highKey": 127,
              "ntr": "rootTransposition", "ntt": "bass" }
          ],
          "retrigger": "pitchShiftToRoot",
          "noteLowLimit": 28, "noteHighLimit": 60,
          "noteMute": [], "chordMute": [],
          "breakpointKey": 48 }
      ]
    },
    {
      "sectionId": "fill_a", "type": "fill", "label": "Fill A",
      "ordinal": 1, "file": "sections/fill_a.mid",
      "lengthBeats": 4, "loop": false, "fillTarget": "self",
      "trackRules": [
        { "trackId": "drums", "enabled": true },
        { "trackId": "bass", "enabled": true,
          "sourceChord": { "root": 0, "type": "maj7" },
          "noteRanges": [
            { "lowKey": 0, "highKey": 127,
              "ntr": "rootTransposition", "ntt": "bass" }
          ],
          "retrigger": "stop",
          "noteLowLimit": 28, "noteHighLimit": 60,
          "noteMute": [], "chordMute": [],
          "breakpointKey": 48 }
      ]
    }
  ],
  "soundMap": [
    { "trackId": "drums", "program": 0, "bankMsb": 127, "bankLsb": 0,
      "gm2Fallback": 0, "internalPreset": "drums/standard_kit" },
    { "trackId": "bass", "program": 33, "bankMsb": 0, "bankLsb": 0,
      "gm2Fallback": 33, "internalPreset": "bass/fingered_standard" }
  ],
  "drumMap": { "base": "gm", "overrides": [] },
  "fx": { "reverbSend": { "drums": 20, "bass": 8 }, "chorusSend": {} },
  "import": {
    "sourceFormat": "yamaha-sff1",
    "sourceFile": "CoolShuffle.sty",
    "adapterVersion": "1.0.0",
    "importedAt": "2026-06-20T10:00:00Z",
    "limitations": [
      { "code": "OTS_SKIPPED", "severity": "info",
        "message": "One Touch Settings chưa được hỗ trợ ở phiên bản này." }
    ],
    "sourceHash": "sha256:abc123…"
  }
}
```

Quy ước rút gọn: `trackRules` của track `isRhythm:true` chỉ cần `{trackId, enabled}` — mọi field transform bị bỏ qua.

---

## 8. ENUM CHORD TYPE (dùng chung cho `sourceChord`, `chordMute`, Chord Engine)

```
maj, maj6, maj7, maj7s11, maj9, maj7_9, maj6_9, aug,
min, min6, min7, min7b5, min9, min7_9, min7_11, minMaj7, minMaj7_9, dim, dim7,
7, 7sus4, 7b5, 7_9, 7s11, 7_13, 7b9, 7b13, 7s9,
maj7aug, 7aug, sus4, sus2, 1plus5, 1plus8
```

34 giá trị — khớp tập chord Yamaha để `chordMute` map 1:1. Chord Engine MVP chỉ *nhận diện* tập con 14 loại (theo brief), nhưng UASF *biểu diễn* đủ 34 để không mất dữ liệu khi import. `root`: 0–11 (C=0). Slash chord biểu diễn ở runtime (Chord Engine output `{root, type, bassNote}`), không nằm trong style data.

---

## 9. VALIDATION RULES (Import Adapter bắt buộc chạy)

**Lỗi chặn (reject import):**
- V01 thiếu `manifest.json` hoặc JSON không parse được
- V02 `formatVersion` lớn hơn engine hỗ trợ và không migration được
- V03 section khai báo nhưng file SMF không tồn tại / SMF hỏng
- V04 không có section `type:main` nào
- V05 track trong `trackRules` không khai báo trong `tracks`
- V06 hai track trùng `midiChannel`
- V07 `noteRanges` chồng lấn hoặc hở (phải phủ kín 0–127, không giao nhau)

**Cảnh báo (import được, hiển thị cho user):**
- W01 `lengthBeats` lệch độ dài SMF thực (đã pad/trim)
- W02 note nằm ngoài `noteLowLimit/HighLimit` trong source MIDI
- W03 drum note không map được sang GM
- W04 thiếu fill cho một main (engine fallback: chuyển section không fill)
- W05 mọi `limitations` severity `warn` từ adapter

**Bất biến runtime (assert trong debug build):** tổng số note-on = note-off sau transform; mọi event timestamp tăng đơn điệu trong queue; retrigger không bao giờ để kẹt note (paired với panic).

---

## 10. VERSIONING & MIGRATION

- `formatVersion` theo semver-major.minor. **Minor** = thêm field optional (engine cũ ignore). **Major** = đổi ngữ nghĩa field (bắt buộc migration code).
- Migration chạy lúc load library, ghi file mới, giữ `source.bin` nguyên vẹn — re-import từ nguồn luôn là đường thoát hiểm khi migration có bug.
- Engine khai báo `supportedFormatVersions: [1.x]`; file mới hơn → thông báo "cần cập nhật app", không cố đọc.

---

## 11. DANH SÁCH KHÔNG HỖ TRỢ v1.0 (công khai với user)

| Mã | Nội dung | Kế hoạch |
|---|---|---|
| OTS_SKIPPED | One Touch Settings | Phase 2 → Performance preset |
| SFF2_GUITAR_NTR_DEGRADED | Guitar voicing NTR | Phase 2 |
| SFF2_MEGAVOICE | Mega Voice articulation | Phase 2/3 (cần internal sound tương ứng) |
| FILL_CROSS | Fill BA/AB cross-fill như section riêng | Engine xử lý transition tương đương; đánh giá lại sau beta |
| AUDIO_STYLE | Style chứa audio part | Phase 3 |
| MULTIPAD | Multi Pad files | Ngoài phạm vi style format |

---

## 12. OPEN QUESTIONS (chốt trong Sprint 0)

1. Thứ tự enum chính xác của NTT/RTR trong binary SFF1 vs SFF2 — xác minh bằng JJazzLab + corpus.
2. `chordMute` bitmask SFF1: xác nhận đủ/đúng thứ tự 34 chord types ở 6.2.
3. Style 3/4, 6/8 và style đổi time signature giữa section — corpus có bao nhiêu %, có cần `timeSignature` per-section không? (Schema dự phòng: cho phép override trong `sections[]` ở v1.1.)
4. SInt SysEx voice setup (XG) → mức độ cần parse để `soundMap` chính xác trên external module XG.
5. Có giữ `source.bin` mặc định bật không? (trade-off dung lượng × khả năng re-import — đề xuất: bật, hỏi user khi library > 2GB.)

---

## 13. NEXT STEP

1. Review spec này → chốt v1.0-rc.
2. Sprint 0 Spike 1 dùng spec làm target: parser `.sty → .uas` cho 10 style mẫu, phát Main A.
3. Viết tiếp **Realtime Arranger Engine Spec** (consumer của UASF): state machine, NTT transform algorithm chi tiết từng bảng, scheduler, retrigger semantics.
