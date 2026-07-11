#ifndef AI_ARRANGER_MIDI_MIDI_INPUT_PARSER_H
#define AI_ARRANGER_MIDI_MIDI_INPUT_PARSER_H

#include <cstddef>
#include <cstdint>

// ── Pure MIDI input parser (symmetric to midiEventToBytes on the output side) ──
//
// Turns a buffer of raw MIDI bytes (as delivered by a CoreMIDI packet, one or
// more complete messages, possibly using running status) into typed channel-voice
// messages. No CoreMIDI dependency, no allocation, no locks — fully unit-testable
// on CI. NoteOn with velocity 0 is normalized to NoteOff (the common keyboard
// convention). System-common / SysEx / realtime bytes are ignored for the chord
// input path.

namespace ai_arranger::midi {

enum class MidiInMsgType : uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    Other,          // program change, pressure, pitch bend, etc. (not chord-relevant)
};

struct MidiInputMessage {
    MidiInMsgType type = MidiInMsgType::Other;
    uint8_t       channel = 0;   // 0-15
    uint8_t       data1 = 0;     // note number or CC number
    uint8_t       data2 = 0;     // velocity or CC value
};

// Parse `len` bytes into up to `maxOut` messages. Returns the count written.
// Robust to running status and partial trailing bytes (an incomplete message at
// the end is dropped, not misread).
size_t parseMidiInput(const uint8_t* bytes, size_t len,
                      MidiInputMessage* out, size_t maxOut) noexcept;

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_MIDI_INPUT_PARSER_H
