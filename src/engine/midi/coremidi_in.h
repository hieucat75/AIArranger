#ifndef AI_ARRANGER_MIDI_COREMIDI_IN_H
#define AI_ARRANGER_MIDI_COREMIDI_IN_H

#include "midi/midi_input_parser.h"     // MidiInputMessage + parseMidiInput
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <CoreMIDI/CoreMIDI.h>

namespace ai_arranger::midi {

struct MidiSourceInfo {
    int         index = -1;
    std::string name;
};

/**
 * Real CoreMIDI input from an external keyboard/controller — the symmetric
 * counterpart to CoreMidiOut.
 *
 * Threading model:
 *   - CoreMIDI delivers packets on its own high-priority read thread
 *     (readProc). readProc parses each packet with the pure parseMidiInput()
 *     and hands every message to the sink. The sink MUST be cheap and lock-free
 *     (it typically just posts a ControlEvent onto the engine's SPSC queue).
 *   - No allocation / locking is done in readProc.
 *
 * CI safety:
 *   - With zero sources (headless CI) initialize() still succeeds and the port
 *     simply receives nothing; selectSource()/hasLiveSource() report "none".
 *   - The parsing/routing logic is fully testable without hardware via
 *     parseMidiInput() + routeMidiInput() (see tests/midi/test_midi_input_parser).
 */
class CoreMidiIn {
public:
    using MessageSink = std::function<void(const MidiInputMessage&)>;

    CoreMidiIn() = default;
    ~CoreMidiIn();

    CoreMidiIn(const CoreMidiIn&) = delete;
    CoreMidiIn& operator=(const CoreMidiIn&) = delete;

    // ── Lifecycle (non-realtime) ─────────────────────────────────────
    bool initialize(const char* clientName = "AI Arranger") noexcept;
    void shutdown() noexcept;
    [[nodiscard]] bool isInitialized() const noexcept;

    // ── Source management (non-realtime) ─────────────────────────────
    [[nodiscard]] std::vector<MidiSourceInfo> enumerateSources() const noexcept;
    [[nodiscard]] int sourceCount() const noexcept;
    // Connect the port to source `index`; -1 disconnects. Returns false if out
    // of range. Remembers the name for hot-plug re-resolution.
    bool selectSource(int index) noexcept;
    [[nodiscard]] int  selectedSource() const noexcept;
    [[nodiscard]] bool hasLiveSource() const noexcept;

    // ── Sink ─────────────────────────────────────────────────────────
    // Invoked for every parsed message on the read thread. Set before selecting.
    void setSink(MessageSink sink) noexcept { sink_ = std::move(sink); }

    [[nodiscard]] uint64_t receivedCount() const noexcept {
        return received_.load(std::memory_order_relaxed);
    }

private:
    static void readProc(const MIDIPacketList* pktList, void* readRefCon, void* srcRefCon);
    static void notifyProc(const MIDINotification* msg, void* refCon);

    MIDIClientRef                client_{0};
    MIDIPortRef                  in_port_{0};
    std::atomic<MIDIEndpointRef> source_{0};   // 0 = none/disconnected
    std::string                  selected_name_;
    std::atomic<int>             selected_index_{-1};

    MessageSink                  sink_{nullptr};
    std::atomic<uint64_t>        received_{0};
    std::atomic<bool>            initialized_{false};
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_COREMIDI_IN_H
