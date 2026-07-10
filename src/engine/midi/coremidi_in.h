#ifndef AI_ARRANGER_MIDI_COREMIDI_IN_H
#define AI_ARRANGER_MIDI_COREMIDI_IN_H

#include "midi/midi_input_source.h"     // IMidiInputSource + MidiSourceInfo + MidiInputMessage
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <CoreMIDI/CoreMIDI.h>

namespace ai_arranger::midi {

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
class CoreMidiIn final : public IMidiInputSource {
public:
    CoreMidiIn() = default;
    ~CoreMidiIn() override;

    CoreMidiIn(const CoreMidiIn&) = delete;
    CoreMidiIn& operator=(const CoreMidiIn&) = delete;

    // ── Lifecycle (non-realtime) ─────────────────────────────────────
    bool initialize(const char* clientName = "AI Arranger") noexcept;
    void shutdown() noexcept;
    [[nodiscard]] bool isInitialized() const noexcept;

    // ── IMidiInputSource ─────────────────────────────────────────────
    std::vector<MidiSourceInfo> enumerateSources() const noexcept override;
    int sourceCount() const noexcept override;
    // Connect the port to source `index`; -1 disconnects. Returns false if out
    // of range. Remembers the name for hot-plug re-resolution.
    bool selectSource(int index) noexcept override;
    int  selectedSource() const noexcept override;
    bool hasLiveSource() const noexcept override;

    // Invoked for every parsed message on the read thread. Set before selecting.
    void setSink(MessageSink sink) noexcept override { sink_ = std::move(sink); }

    uint64_t receivedCount() const noexcept override {
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
