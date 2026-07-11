#ifndef AI_ARRANGER_TESTS_FAKE_MIDI_INPUT_SOURCE_H
#define AI_ARRANGER_TESTS_FAKE_MIDI_INPUT_SOURCE_H

// Synthetic IMidiInputSource for headless tests — no CoreMIDI. inject() parses a
// raw MIDI byte buffer and drives the registered sink exactly as CoreMidiIn's
// read callback would, so the facade can be exercised without hardware.

#include "midi/midi_input_source.h"
#include "midi/midi_input_parser.h"
#include <vector>

namespace ai_arranger::midi {

struct FakeMidiInputSource : IMidiInputSource {
    MessageSink                 sink_;
    std::vector<MidiSourceInfo> devices;
    int      selected_ = -1;
    bool     live_ = false;
    uint64_t received_ = 0;

    void setSink(MessageSink s) noexcept override { sink_ = std::move(s); }
    std::vector<MidiSourceInfo> enumerateSources() const noexcept override { return devices; }
    int  sourceCount() const noexcept override { return static_cast<int>(devices.size()); }
    bool selectSource(int index) noexcept override {
        if (index == -1) { selected_ = -1; live_ = false; return true; }
        if (index < 0 || index >= static_cast<int>(devices.size())) return false;
        selected_ = index; live_ = true; return true;
    }
    int  selectedSource() const noexcept override { return selected_; }
    bool hasLiveSource() const noexcept override { return live_; }
    uint64_t receivedCount() const noexcept override { return received_; }

    // ── test helper ── feed raw MIDI bytes through the parser to the sink
    void inject(const uint8_t* bytes, size_t len) {
        MidiInputMessage m[64];
        const size_t n = parseMidiInput(bytes, len, m, 64);
        for (size_t i = 0; i < n; ++i) { if (sink_) sink_(m[i]); ++received_; }
    }
};

} // namespace ai_arranger::midi

#endif
