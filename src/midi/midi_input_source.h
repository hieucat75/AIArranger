#ifndef AI_ARRANGER_MIDI_MIDI_INPUT_SOURCE_H
#define AI_ARRANGER_MIDI_MIDI_INPUT_SOURCE_H

#include "midi/midi_input_parser.h"   // MidiInputMessage
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ── MIDI input source abstraction (portable core seam) ───────────────────────
//
// The core sees only this interface; the real CoreMIDI implementation lives in
// the Apple platform layer (CoreMidiIn). Lets LiveEngineFacade be driven by a
// fake source in headless tests. Symmetric to IMidiOutputProvider.

namespace ai_arranger::midi {

struct MidiSourceInfo {
    int         index = -1;
    std::string name;
};

class IMidiInputSource {
public:
    virtual ~IMidiInputSource() = default;

    // Invoked for every parsed message (on the source's read thread). Set before
    // selecting a source. Keep it cheap and lock-free (it typically just routes).
    using MessageSink = std::function<void(const MidiInputMessage&)>;
    virtual void setSink(MessageSink sink) noexcept = 0;

    virtual std::vector<MidiSourceInfo> enumerateSources() const noexcept = 0;
    virtual int  sourceCount() const noexcept = 0;
    virtual bool selectSource(int index) noexcept = 0;   // -1 disconnects
    virtual int  selectedSource() const noexcept = 0;
    virtual bool hasLiveSource() const noexcept = 0;
    virtual uint64_t receivedCount() const noexcept = 0;
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_MIDI_INPUT_SOURCE_H
