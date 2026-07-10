#include "midi/midi_input_parser.h"

namespace ai_arranger::midi {

size_t parseMidiInput(const uint8_t* bytes, size_t len,
                      MidiInputMessage* out, size_t maxOut) noexcept {
    if (!bytes || !out || maxOut == 0) return 0;

    size_t  n = 0;
    size_t  i = 0;
    uint8_t status = 0;   // running status (channel-voice only)

    auto emit = [&](MidiInputMessage m) {
        if (n < maxOut) out[n++] = m;
    };

    while (i < len) {
        const uint8_t b = bytes[i];

        if (b & 0x80) {                 // ── status byte ──
            if (b >= 0xF8) { ++i; continue; }              // realtime: ignore, keep running status
            if (b == 0xF0) {                                // SysEx: skip to EOX (0xF7)
                ++i;
                while (i < len && bytes[i] != 0xF7) ++i;
                if (i < len) ++i;                            // consume the 0xF7
                status = 0;
                continue;
            }
            if (b >= 0xF1 && b <= 0xF7) { status = 0; ++i; continue; } // system common: drop
            status = b;                                      // channel-voice status
            ++i;
            continue;
        }

        // ── data byte ──
        if (status == 0) { ++i; continue; }                 // stray data, no running status
        const uint8_t hi = status & 0xF0;
        const uint8_t ch = status & 0x0F;

        if (hi == 0xC0 || hi == 0xD0) {                     // 1 data byte (PC / channel pressure)
            emit({MidiInMsgType::Other, ch, b, 0});
            ++i;
            continue;
        }

        // 2 data bytes (NoteOff/NoteOn/PolyPressure/CC/PitchBend)
        if (i + 1 >= len) break;                            // incomplete trailing message
        const uint8_t d1 = b;
        const uint8_t d2 = bytes[i + 1];
        i += 2;

        MidiInputMessage m;
        m.channel = ch; m.data1 = d1; m.data2 = d2;
        if (hi == 0x90)      m.type = (d2 == 0) ? MidiInMsgType::NoteOff : MidiInMsgType::NoteOn;
        else if (hi == 0x80) m.type = MidiInMsgType::NoteOff;
        else if (hi == 0xB0) m.type = MidiInMsgType::ControlChange;
        else                 m.type = MidiInMsgType::Other; // poly pressure / pitch bend
        emit(m);
    }

    return n;
}

} // namespace ai_arranger::midi
