#ifndef AI_ARRANGER_SESSION_ENGINE_SNAPSHOT_H
#define AI_ARRANGER_SESSION_ENGINE_SNAPSHOT_H

#include <cstdint>

// ── UI-facing engine snapshot (portable, POD) ────────────────────────────────
//
// A trivially-copyable value the facade publishes each tick and the UI polls at
// ~30-60 Hz. No pointers, no containers — safe to store in std::atomic and to
// marshal across the Swift/C++ boundary as a plain struct.

namespace ai_arranger::session {

struct EngineSnapshot {
    bool     playing = false;
    uint8_t  chordRoot = 0;      // MIDI note (0 = C-1); meaningful only with a chord
    uint8_t  chordType = 0;      // arranger::ChordType
    uint8_t  chordBass = 0;
    int32_t  section = 0;        // current section index
    uint32_t tempoBpm = 120;
    int64_t  positionTicks = 0;
    int32_t  performerState = 0; // performance::PerformerState
    bool     midiInLive = false;
    bool     midiOutLive = false;
    uint64_t receivedMessages = 0;
    uint64_t dispatchedNotes = 0;
};

} // namespace ai_arranger::session

#endif // AI_ARRANGER_SESSION_ENGINE_SNAPSHOT_H
