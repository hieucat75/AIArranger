# REALTIME ARRANGER ENGINE — SPECIFICATION v0.9
**Module 4 — "Bộ não" sản phẩm — Trạng thái: Draft — Consumer của UASF Spec v0.9**

---

## 1. PHẠM VI & TRÁCH NHIỆM

Engine nhận: (a) UASF style đã load, (b) lệnh transport/section từ UI, (c) hợp âm realtime từ Chord Engine. Engine xuất: dòng MIDI event đã transform, timestamp sample-accurate, tới MIDI Engine và/hoặc Sound Engine.

**Engine chịu trách nhiệm:**
- Đồng hồ nhạc (musical clock) và transport.
- State machine section (intro/main/fill/break/ending + quantized switching).
- Đọc note gốc từ section MIDI, áp NTR/NTT transform theo hợp âm hiện tại.
- Retrigger semantics khi đổi hợp âm.
- Lookahead scheduling, lock-free, không block audio thread.
- Panic / all-notes-off, MIDI disconnect handling.

**Engine KHÔNG chịu trách nhiệm:** nhận diện hợp âm (Chord Engine), routing/output (MIDI Engine), tạo âm thanh (Sound Engine), parse style (Import Adapter).

---

## 2. MÔ HÌNH THREADING

```
┌── UI thread (60Hz) ─────────────────────────┐
│  - đẩy Command vào commandQueue (lock-free)  │
│  - đọc EngineState snapshot (atomic) để vẽ   │
└──────────────┬──────────────────────────────┘
               │ lock-free SPSC queue (commands)
               │ atomic double-buffer (state out)
┌──────────────▼──────────────────────────────┐
│  Audio thread (CoreAudio render callback)    │
│  ── KHÔNG malloc, KHÔNG lock, KHÔNG file IO ──│
│  1. drain commandQueue                        │
│  2. drain chordQueue (từ Chord Engine)        │
│  3. advance clock theo buffer frames          │
│  4. process section state machine             │
│  5. render events trong lookahead window      │
│  6. transform + push ra outputQueue           │
│  7. publish EngineState snapshot              │
└──────────────┬──────────────────────────────┘
               │ event buffer (sample-accurate)
        ┌──────▼──────┐     ┌──────────────┐
        │ MIDI Engine │     │ Sound Engine │
        └─────────────┘     └──────────────┘
```

**Quy tắc bất khả xâm phạm:**
1. Audio thread không bao giờ cấp phát bộ nhớ. Mọi buffer (event pool, voice state, note-tracking) pre-allocate lúc load style.
2. Mọi giao tiếp với audio thread qua lock-free queue (SPSC ring buffer) hoặc atomic.
3. Style load/swap xảy ra ở thread khác; con trỏ style mới chuyển vào audio thread bằng atomic pointer swap + RCU-style reclaim (style cũ giải phóng sau khi audio thread xác nhận đã đổi).

---

## 3. MUSICAL CLOCK (nền tảng chống jitter)

**Nguồn thời gian duy nhất = vị trí sample của audio render.** Không timer OS, không `DispatchSourceTimer`.

Trạng thái clock (cập nhật mỗi callback, `nFrames` = số frame buffer):

```
ticksPerSample = (tempoBpm / 60.0) * ppq / sampleRate
// đầu callback:
blockStartTick = currentTick
blockEndTick   = currentTick + ticksPerSample * nFrames
// cuối callback:
currentTick   += ticksPerSample * nFrames
```

`currentTick` là `double` (tích lũy chính xác, tránh trôi). Mọi event được lập lịch theo tick → quy đổi ngược ra frame offset trong buffer:

```
frameOffset = round((eventTick - blockStartTick) / ticksPerSample)
clamp(frameOffset, 0, nFrames-1)
```

**Tap tempo & tempo change:** đổi `tempoBpm` chỉ thay `ticksPerSample` ở callback kế — tick tích lũy liên tục, không nhảy. Tap tempo: UI gửi command với timestamp; engine tính BPM trung bình động 4 tap gần nhất, làm mượt (không nhảy quá ±X% mỗi lần để tránh giật).

**MIDI clock out:** phát 24 PPQN từ `currentTick`, timestamp sample-accurate như mọi event khác.

---

## 4. LOOKAHEAD SCHEDULING

Mỗi callback, engine render mọi event trong cửa sổ `[blockStartTick, blockEndTick + lookaheadTicks)` nhưng **chỉ output** phần thuộc buffer hiện tại; phần tương lai gần được tính sẵn để quyết định section boundary/retrigger kịp thời.

Thực tế đơn giản hóa cho v1.0: **lookahead = đúng 1 buffer** (render events thuộc `[blockStartTick, blockEndTick)`), vì:
- CoreMIDI nhận `MIDITimeStamp` tương lai → ta gửi event với host-time = thời điểm frame đó sẽ phát ra loa, độ chính xác do CoreMIDI đảm bảo, không cần lookahead nhiều buffer.
- Sound Engine nội bộ nhận `frameOffset` trong buffer → sample-accurate sẵn.

Lookahead nhiều buffer chỉ cần khi muốn "nhìn trước" để chuẩn bị (vd cross-fade section) — đánh dấu là tối ưu phase 2, không phải MVP.

**Cấu trúc render mỗi section:** section MIDI được "phẳng hóa" lúc load thành mảng event sort theo tick (đã tách Note On/Off thành cặp có pointer). Con trỏ đọc `playCursor` (index trong mảng) tiến theo tick. Loop section: khi `currentTick >= sectionLengthTick`, wrap về 0 (hoặc nhảy boundary, xem state machine).

---

## 5. SECTION STATE MACHINE

### 5.1. Trạng thái

```
STOPPED → (Start/SyncStart) → COUNT_IN? → PLAYING_INTRO?
        → PLAYING_MAIN ⇄ (Fill/Break) → PLAYING_ENDING → STOPPED
```

```
enum PlayState { STOPPED, COUNT_IN, PLAYING }
struct EngineState {
  PlayState play;
  SectionId current;        // section đang phát
  SectionId pending;        // section đã đặt lệnh, chờ boundary (hoặc NONE)
  double currentTick;
  int    currentBar, currentBeat;
  Chord  chord;             // hợp âm hiện hành
  bool   autoFill;
}
```

### 5.2. Quy tắc quantize chuyển section (mô phỏng hành vi Yamaha)

| Lệnh | Thời điểm commit | Ghi chú |
|---|---|---|
| Start (từ STOPPED) | ngay (hoặc sau count-in) | bắt đầu intro nếu chọn, không thì main |
| Sync Start | tại note hợp âm đầu tiên user bấm | |
| Main A→B (đổi variation) | **cuối bar hiện tại** | nếu `autoFill` → chèn Fill tương ứng rồi vào Main đích |
| Fill (thủ công) | **cuối beat hiện tại** | fill chơi hết độ dài rồi quay về main (`fillTarget`) |
| Break | cuối beat hiện tại | |
| Intro | chỉ hợp lệ khi vừa Start, không loop | |
| Ending | **cuối bar hiện tại**; ending chạy hết → STOPPED | nhấn Ending lần 2 khi đang ending → ending dài hơn (ordinal cao hơn) nếu có |
| Stop | cuối bar (Sync Stop) hoặc ngay (Instant Stop) | cấu hình được |

**Cơ chế pending:** lệnh từ UI set `pending`. Mỗi callback, sau khi advance clock, engine kiểm tra: nếu vượt qua boundary phù hợp với loại `pending` → commit (đổi `current`, reset `playCursor`, clear `pending`). UI hiển thị `pending` nhấp nháy để nhạc công biết "đã nhận, đang chờ nhịp".

### 5.3. Auto Fill

Khi `autoFill = true` và user chuyển Main A→B: engine tự chèn Fill của main nguồn (hoặc fill gần nhất) trước khi vào Main B, commit tại cuối bar. Nếu fill dài hơn phần còn lại của bar → fill bắt đầu ngay cuối beat để kịp.

### 5.4. Xử lý chuyển section & note đang vang

Khi commit section mới: mọi note-on chưa tắt của section cũ phải được giải quyết:
- Track rhythm (drum/perc): gửi Note Off ngay tại boundary (sạch, không kéo đuôi sang section mới).
- Track pitched: áp dụng retrigger policy của **section mới** (thường note section cũ tắt, section mới tự phát note đầu) — tránh kẹt note. Panic-safe: mọi note-on được track trong `activeNotes` set, boundary luôn flush phần không được section mới tiếp quản.

---

## 6. CHORD TRANSFORM — TRÁI TIM ENGINE

Đây là phần biến note gốc (theo `sourceChord`, mặc định CMaj7) thành note theo hợp âm user bấm, theo `noteRanges[].ntr` và `.ntt`.

### 6.1. Pipeline cho mỗi Note On của track pitched

```
input: srcNote (0-127), trackRule, currentChord {root, type, bassNote}
1. range = chọn noteRange chứa srcNote (theo lowKey/highKey)
2. nếu chordMute chứa currentChord.type → bỏ qua note (không phát)
3. nếu noteMute chứa pitchClass(srcNote) → bỏ qua
4. transposed = applyNTR(srcNote, range.ntr, sourceChord.root, currentChord.root)
5. mapped = applyNTT(transposed, range.ntt, sourceChord, currentChord)
6. limited = applyNoteLimit(mapped, noteLowLimit, noteHighLimit, range.breakpointKey)
7. output Note On(limited)  + lưu (srcNote → limited) vào activeMap để Note Off khớp
```

**Note Off** phải dùng cùng ánh xạ đã lưu trong `activeMap` (vì transform phụ thuộc hợp âm tại thời điểm Note On; nếu hợp âm đổi giữa chừng, retrigger logic ở §7 quyết định, nhưng Note Off của note CŨ luôn tắt đúng pitch đã phát).

### 6.2. NTR — Note Transposition Rule

**`rootTransposition`:** dịch note theo độ lệch root.
```
delta = (currentChord.root - sourceChord.root) mod 12   // 0..11
transposed = srcNote + delta
// đưa về gần breakpoint để không nhảy quãng tám vô lý:
while transposed > breakpointKey: transposed -= 12
while transposed <= breakpointKey - 12: transposed += 12
```
Dùng cho bass, phrase giai điệu — giữ contour, dịch theo root.

**`rootFixed`:** không dịch theo root; note giữ nguyên cao độ tương đối rồi remap vào chord tone (qua NTT). Dùng cho pad/chord track nơi không muốn toàn bộ nhảy theo root mà muốn "rải" theo hợp âm. Thực tế: `transposed = srcNote` rồi để NTT làm việc remap.

**`guitar` (SFF2):** v1.0 fallback về `rootTransposition` + limitation log (đã chốt ở UASF spec).

### 6.3. NTT — Note Transposition Table (phần tinh tế nhất)

NTT quyết định, sau khi đã xử lý root, mỗi note được ánh xạ vào thang/hợp âm đích ra sao khi **loại** hợp âm đổi (vd CMaj7 → Cm7 → C7sus4). Mô hình hóa bằng bảng 12 phần tử: với mỗi pitch-class của note nguồn (tương đối so với source chord root), trả về pitch-class đích (tương đối so với current chord root), theo current chord type.

**Cấu trúc dữ liệu (pre-compute lúc load):**
```
// Với mỗi NTT mode và mỗi chord type đích, có bảng map[12] → 12
nttTable[mode][chordType][srcPitchClassRel] = dstPitchClassRel
```

**Các mode (khớp enum UASF):**

- **`bypass`**: không remap theo type, chỉ chịu NTR. `map[i] = i`. Dùng khi muốn giữ nguyên (vd một số phrase).

- **`melody`**: ánh xạ giai điệu vào hợp âm đích, ưu tiên giữ chord tone là chord tone, tension dịch tối thiểu (nearest chord/scale tone). Bảng được dựng từ: chord tone nguồn → chord tone đích gần nhất; non-chord tone → giữ nếu thuộc scale đích, không thì dịch nửa cung về tone gần nhất.

- **`chord`**: mọi note ép vào chord tone đích (dùng cho pad/chord backing). Non-chord tone snap về chord tone gần nhất theo loại hợp âm đích.

- **`bass`**: tối ưu cho line bass — root nguồn → root đích, fifth → fifth đích, các note khác theo scale bậc tương ứng; tránh tạo nốt nghịch với bassNote (slash chord: nếu có `bassNote`, note bass thấp nhất ưu tiên = bassNote).

- **`melodicMinor` / `harmonicMinor` / `naturalMinor` / `dorian`** (+ biến thể `…B5`): chọn thang tương ứng làm đích khi remap non-chord tone; biến thể `B5` hạ bậc 5 (cho hợp âm có ♭5/dim).

**Thuật toán dựng bảng (offline, lúc load style hoặc precompute toàn cục):**
```
for chordType in allChordTypes:
  chordTones = intervalsOf(chordType)          // vd maj7 → {0,4,7,11}
  scale      = scaleFor(nttMode, chordType)    // thang đích
  for pc in 0..11:
    if pc in sourceChordTones:
       dst = mapChordToneByDegree(pc, sourceChordType, chordType)
    elif pc in scale:
       dst = pc                                 // giữ tension hợp lệ
    else:
       dst = nearest(pc, chordTones ∪ scale)    // snap nửa cung
    nttTable[mode][chordType][pc] = dst
```

> ⚠️ **Độ tin cậy:** bảng NTT chính xác của Yamaha là bí mật thương mại và có sắc thái âm nhạc tinh tế. Cách tiếp cận: dựng bảng theo lý thuyết nhạc (như trên) làm baseline, rồi **hiệu chỉnh bằng A/B test với corpus + tai nhạc công** trong Sprint 1–2. Không claim khớp 100% Yamaha; mục tiêu MVP là "nghe đúng và mượt", tinh chỉnh dần. Đây là chỗ AI humanization (phase 2) có thể tăng chất lượng.

### 6.4. Note Limit
```
while mapped > noteHighLimit: mapped -= 12
while mapped < noteLowLimit:  mapped += 12
```
Giữ note trong dải nhạc cụ thật (vd bass 28–60). Áp sau NTT.

---

## 7. RETRIGGER SEMANTICS (cảm giác "mượt" của arranger)

Khi hợp âm đổi **giữa lúc note pitched đang vang**, mỗi track xử lý theo `retrigger` của section/track hiện tại. Đây là điểm phân biệt arranger tốt vs dở.

| Policy | Hành vi note đang vang khi đổi hợp âm |
|---|---|
| `stop` | Tắt note ngay (Note Off). Note mới chỉ phát khi section MIDI có Note On kế tiếp. Dùng cho phrase ngắt quãng. |
| `pitchShift` | Note đang vang **đổi cao độ** sang note đã transform theo hợp âm mới, không ngắt tiếng (Note Off cũ + Note On mới tức thời, hoặc dùng mono-legato nếu sound engine hỗ trợ). Mượt, dùng cho pad. |
| `pitchShiftToRoot` | Như trên nhưng note **chuyển về root** của hợp âm mới. Dùng cho bass — đổi hợp âm thì bass nhảy về nốt gốc ngay. |
| `retrigger` | Tắt rồi **đánh lại** note (Note Off + Note On mới) ở cao độ đã transform — nghe rõ "cú đánh lại". |
| `retriggerToRoot` | Đánh lại ở root hợp âm mới. |
| `noteGenerator` | Sinh note mới theo hợp âm dù section MIDI tại điểm đó không có note (giữ hợp âm "dày"). Phức tạp; v1.0 fallback về `pitchShift` + limitation. |

**Cài đặt:** khi nhận chord-change event tại `tick T`:
```
for mỗi note đang vang trong activeMap của track pitched:
   newTarget = transform(srcNote, newChord)     // §6 pipeline
   switch retrigger:
     stop:             emit NoteOff(oldPlayed)
     pitchShift:       emit NoteOff(oldPlayed); emit NoteOn(newTarget, sameVel)
     pitchShiftToRoot: emit NoteOff(oldPlayed); emit NoteOn(rootNote, sameVel)
     retrigger:        emit NoteOff(oldPlayed); emit NoteOn(newTarget, freshVel)
     ...
   cập nhật activeMap[srcNote] = newPlayed
```
Tất cả emit tại `tick T` với frame offset chính xác → đổi hợp âm "ăn" ngay tại nhịp người chơi bấm, đây là yêu cầu chord-recognition < 20ms ở brief (engine phần này gần như 0ms; độ trễ chủ yếu ở input + buffer).

**Chord change timing:** mặc định áp ngay (realtime). Tùy chọn `chordQuantize` (off / beat / bar) cho người chơi muốn hợp âm chỉ đổi đúng phách — set ở performance preset, mặc định off.

---

## 8. RENDER LOOP (pseudocode audio callback)

```
void renderBlock(float* out, int nFrames):
  // 1. Lệnh điều khiển
  while cmd = commandQueue.pop():
     applyCommand(cmd)            // start/stop/section/tempo/panic...

  // 2. Hợp âm mới
  while ch = chordQueue.pop():
     state.chord = ch
     applyRetriggerToActiveNotes(ch)   // §7, emit tại frame của ch.timestamp

  // 3. Tiến clock
  blockStartTick = currentTick
  currentTick   += ticksPerSample * nFrames
  blockEndTick   = currentTick

  // 4. State machine: kiểm tra boundary, commit pending
  processSectionBoundaries(blockStartTick, blockEndTick)

  // 5. Render events trong [blockStartTick, blockEndTick)
  while ev = section.eventAt(playCursor) and ev.tick < blockEndTick:
     if ev.tick < blockStartTick: ev.tick = blockStartTick   // catch-up
     frame = toFrame(ev.tick, blockStartTick)
     if track(ev).isRhythm:
        emit(ev, frame)                          // drum: không transform
     else:
        transformed = transformPipeline(ev, state.chord)   // §6
        if transformed != MUTED: emit(transformed, frame)
     playCursor++
     if playCursor == section.end:
        handleLoopOrBoundary()                   // loop main / kết thúc intro...

  // 6. MIDI clock out (24 PPQN)
  emitMidiClockPulses(blockStartTick, blockEndTick)

  // 7. Publish state cho UI (atomic)
  publishState()
```

`emit` đẩy vào outputQueue (sample-accurate, đã transform). MIDI Engine và/hoặc Sound Engine tiêu thụ. Audio callback của Sound Engine và của transport là cùng một callback (đồng hồ mẫu chung) → không lệch.

---

## 9. PANIC & FAULT HANDLING

- **Panic / All Notes Off:** lệnh ưu tiên cao nhất, xử lý đầu commandQueue. Gửi Note Off cho mọi note trong `activeNotes` (tracking đầy đủ, không dựa CC123 vì module ngoài không phải lúc nào cũng nghe), rồi CC123 + CC120 trên mọi channel làm lớp bảo hiểm. Reset sustain (CC64=0).
- **MIDI device disconnect:** MIDI Engine báo engine; engine không dừng transport (nhạc công đang diễn!), chuyển output sang internal sound nếu cấu hình hybrid, hiển thị cảnh báo. Note đang gửi ra device mất → activeNotes cleanup để không kẹt khi cắm lại.
- **CPU overload (callback trễ):** phát hiện qua deadline miss; ưu tiên giữ transport đúng (drop event thừa hơn là vỡ nhịp); log để hậu kiểm. Không bao giờ spin/lock trong callback.
- **Style swap khi đang chơi:** chỉ cho phép tại STOPPED ở v1.0 (đổi style khi đang chơi = phase 2, cần cross-fade). Setlist preload next style vào RAM nhưng activate khi stop.

---

## 10. INTERFACE (C++ core ↔ Swift)

API surface mỏng, mọi thứ qua command/state (không gọi hàm trực tiếp vào audio thread):

```cpp
class ArrangerEngine {
public:
  // gọi từ UI thread — chỉ enqueue
  void postCommand(Command);           // Start/Stop/SetSection/Fill/Break/
                                       // Ending/SetTempo/TapTempo/Panic/
                                       // SetAutoFill/SetChordQuantize...
  void postChord(Chord, HostTime);     // từ Chord Engine (có thể chính là
                                       // audio thread nếu chord input MIDI)
  EngineState snapshot() const;        // atomic, cho UI 60Hz

  // gọi từ loader thread
  void loadStyle(const UASFStyle*);    // atomic swap
  void prepare(double sampleRate, int maxBlock);  // pre-allocate

  // gọi từ audio thread (bởi host)
  void renderBlock(...);
};
```

`Command` là POD (trivially copyable) để đẩy qua lock-free queue an toàn.

---

## 11. KIỂM THỬ (gắn với Testing Strategy)

- **Unit:** transformPipeline với bảng vector đầu vào/đầu ra cố định cho mỗi NTR×NTT×chordType; chord-mute/note-mute; note-limit wrap.
- **Property test:** với mọi chuỗi chord ngẫu nhiên + section ngẫu nhiên, bất biến: #NoteOn == #NoteOff cuối cùng (không kẹt note); mọi tick output đơn điệu tăng; không note ngoài [0,127].
- **Timing stress (CI):** chạy offline render 30 phút nhạc, đổi section 1000 lần, assert: không event lệch quá ±1 frame so với lịch lý thuyết; loop boundary chính xác mẫu.
- **A/B nhạc tính:** render cùng style + cùng chuỗi hợp âm, so sánh với playback Yamaha tham chiếu (thu audio), nhạc công chấm điểm — tinh chỉnh bảng NTT.
- **Latency thật:** đo từ MIDI note-in (hợp âm) tới audio-out trên iPad thật, buffer 128/256, mục tiêu < 20ms (out MIDI) / < 30ms (internal).

---

## 12. OPEN QUESTIONS (chốt trong Sprint 1)

1. Bảng NTT baseline có cần tách riêng cho từng họ chord (maj/min/dom/dim) hay một thuật toán tổng quát đủ tốt? → quyết sau A/B test.
2. `noteGenerator` retrigger: có style phổ biến nào dựa vào nó không? Nếu hiếm, giữ fallback `pitchShift` lâu dài.
3. Chord change ngay giữa fill: áp transform lên fill (pitched) hay khóa hợp âm đến hết fill? Đề xuất: áp ngay (nhất quán), test cảm giác.
4. Sync Stop vs Instant Stop mặc định nào cho nhạc công VN? → hỏi beta.
5. Có cần "manual bass" (giữ bass theo nốt thấp tay trái thay vì root hợp âm) trong MVP? Brief liệt kê — đề xuất có, vì rẻ (chỉ override `bass` track root = bassNote nhập tay).

---

## 13. NEXT STEP

1. Review spec → chốt cùng UASF v1.0-rc.
2. Sprint 0: implement clock + render loop + transform pipeline tối thiểu (NTR rootTransposition + NTT bass/chord) → đạt milestone "phát Main A đúng hợp âm qua MIDI out".
3. Tài liệu kế tiếp đề xuất: **Chord Recognition Engine Spec** (consumer đứng trước engine này) hoặc **MIDI Engine Spec** (consumer đứng sau). Đề xuất Chord Engine trước vì nó đóng vòng "bấm hợp âm → nghe style đổi" cho demo Sprint 0.
