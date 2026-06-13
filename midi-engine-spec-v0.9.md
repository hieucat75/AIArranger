# MIDI ENGINE — SPECIFICATION v0.9
**Module 6 — Output & Input Router — Trạng thái: Draft**

---

## 1. PHẠM VI & VỊ TRÍ

MIDI Engine là lớp I/O nằm giữa core engine và thế giới bên ngoài:

```
Arranger Engine (outputQueue)
        ↓
   MIDI Engine
   ├── Route → CoreMIDI device (USB / BLE / Network)
   ├── Route → Virtual MIDI port (cho AUM, GarageBand, DAW)
   └── Route → Sound Engine nội bộ (sfizz)

Nguồn MIDI input:
   CoreMIDI device → InputRouter
   ├── note < splitPoint → Chord Engine
   ├── note ≥ splitPoint → Right Hand (pass-through / Sound Engine)
   └── CC / PC / SysEx → Control Handler
```

MIDI Engine **không** biến đổi nội dung event (transform là việc của Arranger Engine). Nó chỉ: nhận event đã timestamp → gửi đúng device đúng thời điểm, và nhận event đến → phân loại route.

---

## 2. OUTPUT MODES

| Mode | Mô tả | Dùng khi |
|---|---|---|
| `externalOnly` | Toàn bộ output gửi ra CoreMIDI device | Dùng sound module/keyboard ngoài |
| `internalOnly` | Toàn bộ output vào Sound Engine nội bộ | iPad standalone |
| `hybrid` | Per-track cấu hình: external hoặc internal | Drum ngoài + pad nội bộ |
| `virtualMidi` | Mở Virtual MIDI port, app khác nhận | Dùng AUM/MainStage làm synth |

Mode mặc định: `internalOnly` (app chạy được mà không cần thiết bị ngoài).

---

## 3. DEVICE MANAGEMENT

### 3.1. Loại device CoreMIDI

| Loại | Latency | Ghi chú |
|---|---|---|
| USB MIDI (class-compliant) | Thấp nhất ~1–3ms | Khuyến nghị cho live show |
| Apple BLE MIDI | 5–15ms, variable | Cảnh báo tự động khi chọn |
| Network MIDI (RTP-MIDI) | Phụ thuộc mạng | Dùng trong studio, không khuyến nghị live |
| Virtual MIDI port (outbound) | ~0ms | Gửi tới app khác trên cùng thiết bị |

### 3.2. Device Enumeration & Hot-plug

```cpp
class MidiDeviceManager {
  void scanDevices();                          // gọi khi app start + khi CoreMIDI notify
  List<MidiDevice> availableOutputs() const;
  List<MidiDevice> availableInputs()  const;
  void selectOutput(DeviceId, TrackId);        // per-track hoặc global
  void onDeviceAdded(MidiDevice);              // CoreMIDI notification
  void onDeviceRemoved(DeviceId);              // → trigger reconnect UI
};
```

Hot-plug xử lý:
- Device thêm vào → notify UI, không tự-connect (tránh bất ngờ trong show).
- Device rút ra → **không dừng transport**. MIDI Engine chuyển track đó sang `internalOnly` tạm thời, hiển thị cảnh báo màu đỏ trên UI. Nhạc công quyết định reconnect hay chuyển mode.
- Reconnect: user tap "Reconnect" → engine gửi lại toàn bộ init sequence (PC/Bank/CC) cho device.

### 3.3. Virtual MIDI Port

Khi `virtualMidi` bật, engine tạo 1 port input + 1 port output với tên app (vd "Live Arranger"). AUM, GarageBand, Drambo nhìn thấy ngay. Đây là feature community iOS music rất cần và gần như miễn phí (CoreMIDI API sẵn).

---

## 4. OUTPUT PIPELINE

### 4.1. Flow mỗi event

```
Arranger Engine enqueue: TimestampedMidiEvent {
  uint8_t   data[3];
  uint8_t   length;        // 1–3
  uint64_t  hostTime;      // CoreAudio host time của frame
  TrackId   trackId;
}

MIDI Engine nhận từ outputQueue:
  1. Tra routingTable[trackId] → OutputTarget (device + channel override)
  2. Nếu target = CoreMIDI device:
       MIDIPacket.timeStamp = event.hostTime   // gửi tương lai, CoreMIDI lo đúng giờ
       MIDISend(outputPort, endpoint, packetList)
  3. Nếu target = Sound Engine:
       soundEngine.scheduleEvent(event.data, event.hostTime)
  4. Nếu target = Virtual port:
       như CoreMIDI device, endpoint = virtualOutputEndpoint
```

**Không bao giờ** gọi `MIDISend` từ audio thread trực tiếp — đẩy vào một MIDI send queue, một thread riêng drain và gọi CoreMIDI. CoreMIDI `MIDISend` với `MIDITimeStamp` tương lai là thread-safe và đúng giờ.

### 4.2. Routing Table

```json
{
  "routing": [
    { "trackId": "drums",   "output": "device:USB-001", "channel": 9  },
    { "trackId": "bass",    "output": "device:USB-001", "channel": 11 },
    { "trackId": "pad",     "output": "internal",       "channel": 12 },
    { "trackId": "phrase1", "output": "virtual",        "channel": 15 }
  ]
}
```

Lưu trong Performance Preset. UI mixer cho phép kéo thả route per-track.

---

## 5. INITIALIZATION SEQUENCE

Khi load style (hoặc reconnect device), MIDI Engine gửi init sequence theo thứ tự:

```
1. GM/GS/XG Reset (tuỳ loại module, cấu hình được):
   GM:  0xF0 0x7E 0x7F 0x09 0x01 0xF7
   XG:  0xF0 0x43 0x10 0x4C 0x00 0x00 0x7E 0x00 0xF7
   GS:  0xF0 0x41 0x10 0x42 0x12 0x40 0x00 0x7F 0x00 0x41 0xF7

2. Với mỗi track (theo soundMap trong UASF manifest):
   a. Bank Select MSB: CC#0  channel, bankMsb
   b. Bank Select LSB: CC#32 channel, bankLsb
   c. Program Change:  PC    channel, program
   d. Volume:          CC#7  channel, defaultVolume
   e. Pan:             CC#10 channel, defaultPan
   f. Reverb Send:     CC#91 channel, reverbSend   (từ fx hints)
   g. Chorus Send:     CC#93 channel, chorusSend
   h. Expression:      CC#11 channel, 127
   i. Sustain Off:     CC#64 channel, 0

3. Chờ 100ms trước khi phát note đầu tiên
   (một số module cần thời gian load voice)
```

Reset mode (GM/GS/XG/None) cấu hình được per-device trong Settings. Mặc định: GM.

---

## 6. MIDI INPUT ROUTING

```
CoreMIDI inputCallback(MIDIPacketList):
  for each packet:
    msg = parse(packet.data)
    ts  = packet.timeStamp

    switch msg.type:
      NoteOn/NoteOff:
        if msg.note < splitPoint:
          chordEngine.pushNote(msg.note, msg.velocity, ts)
        else:
          rightHandQueue.push(msg, ts)   // bypass, phase 2

      ControlChange:
        controlHandler.process(msg, ts)
        // CC64 sustain → chord engine hold mode
        // CC7 volume   → mixer (nếu external controller mapped)
        // CC123 → panic trigger

      ProgramChange / BankSelect:
        // nếu từ external controller → ignore hoặc map ke Performance
        // cấu hình được

      SysEx:
        // bỏ qua (không relay SysEx từ keyboard vào style output)

      Clock/Start/Stop/Continue (MIDI clock in):
        if clockSource == external:
          clockEngine.onExternalClock(ts)  // phase 2
```

### 6.1. Sustain Pedal (CC64)

Khi sustain ON: chord engine bật `holdMode` tạm thời (giữ chord khi nhả tay). Khi sustain OFF: tắt hold, emit chord thực tế đang giữ (hoặc CHORD_NONE nếu nhả hết).

Hành vi này là convention chuẩn của arranger keyboard — nhạc công đã quen.

### 6.2. Expression Pedal (CC11)

Map vào master volume Arranger hoặc volume track cụ thể (cấu hình trong Performance Preset). Không ảnh hưởng chord engine.

---

## 7. MIDI CLOCK OUT

```
24 PPQN: emit 1 clock byte (0xF8) mỗi ppq/24 tick
Start:   0xFA khi transport start
Stop:    0xFC khi transport stop
Continue:0xFB khi resume từ pause
```

Timestamp clock bytes chính xác theo audio clock (§3 Arranger Engine Spec) → clock out jitter < 1 sample frame (~0.02ms @ 44.1kHz). Đây là clock chất lượng cao hơn nhiều so với timer-based clock.

Bật/tắt MIDI clock out trong Settings (mặc định: tắt — tránh conflict khi dùng với DAW có clock riêng).

---

## 8. PANIC & ALL NOTES OFF

Priority cao nhất — xử lý trước mọi thứ khác trong send queue.

```cpp
void panic():
  // 1. Flush send queue
  sendQueue.clear()

  // 2. Gửi Note Off cho mọi note đang track
  for note in activeNotes:
    sendNoteOff(note.channel, note.pitch, 0)
  activeNotes.clear()

  // 3. CC safety net trên mọi channel (0–15)
  for ch in 0..15:
    send(CC, ch, 123, 0)   // All Notes Off
    send(CC, ch, 120, 0)   // All Sound Off
    send(CC, ch,  64, 0)   // Sustain Off
    send(CC, ch,  11, 127) // Expression reset

  // 4. Gửi ngay, không delay, không hostTime tương lai
  flushImmediate()
```

Panic button trên UI luôn visible, không bị che bởi bất kỳ overlay nào. Minimum touch target 80×80pt.

---

## 9. ACTIVE NOTE TRACKING

```cpp
struct ActiveNote {
  uint8_t channel;
  uint8_t pitch;
  uint8_t velocity;
  TrackId trackId;
};
CircularBuffer<ActiveNote, 512> activeNotes;
```

Mỗi Note On → push. Mỗi Note Off (hoặc matching Off từ Arranger Engine) → pop. Panic flush toàn bộ. Đây là "insurance" — kể cả khi có bug tạo orphan Note On, panic luôn dọn sạch.

---

## 10. BLE MIDI — CẢNH BÁO ĐẶC BIỆT

BLE MIDI trên iOS có latency variable 5–15ms và đôi khi packet loss. Engine xử lý:
- Hiển thị cảnh báo khi user chọn BLE device: "BLE MIDI có độ trễ cao hơn USB. Khuyến nghị dùng USB adapter cho live show."
- Không disable BLE — có trường hợp dùng hợp lệ (wireless controller, không thể cắm dây).
- Không có buffer compensation đặc biệt cho BLE ở v1.0 — accept limitation, document rõ.

---

## 11. INTERFACE C++

```cpp
class MidiEngine {
public:
  // Setup
  void prepare(double sampleRate, int maxBlock);
  void setRouting(TrackId, OutputTarget);
  void setResetMode(DeviceId, ResetMode);  // GM/GS/XG/None

  // Runtime (gọi từ audio/MIDI send thread)
  void processOutputQueue();               // drain outputQueue từ Arranger Engine
  void sendInitSequence(DeviceId);         // gọi sau connect/reconnect

  // Input (gọi từ CoreMIDI callback)
  void onMidiInput(const MIDIPacketList*);

  // Control (gọi từ UI thread)
  void panic();
  void sendRawMessage(DeviceId, uint8_t*, int len);  // diagnostics

  // Device management
  MidiDeviceManager& deviceManager();
};
```

---

## 12. OPEN QUESTIONS (Sprint 0–1)

1. Virtual MIDI port: expose 1 port (all tracks mix) hay 16 port riêng per-track? Đề xuất v1.0: 1 port (đủ cho 90% use case), phase 2 thêm multi-port.
2. MIDI clock in (sync từ DAW/drum machine): có cần ở MVP không? Đề xuất: không, phase 2. Nhưng clock in architecture phải sẵn trong ClockEngine.
3. SysEx forwarding từ keyboard → có user case nào cần relay SysEx vào style output không? Đề xuất: không, tắt hoàn toàn ở MVP.
4. Khi hybrid mode: nếu external device disconnect giữa show, có tự fallback sang internal không? Đề xuất: có, kèm cảnh báo rõ.
