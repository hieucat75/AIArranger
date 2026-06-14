#ifndef AI_ARRANGER_SESSION_SNAPSHOT_MANAGER_H
#define AI_ARRANGER_SESSION_SNAPSHOT_MANAGER_H

#include "session/engine_session.h"
#include <cstdint>
#include <string>

// ── Snapshot state system (Gate 14 Task J) ───────────────────────────
//
// Captures and restores performer/session state (variation, tempo, split,
// manual bass, sync, chord-scan mode index). Serializes to a small JSON string
// and validates on the way back in (version + ranges) before applying. No audio.

namespace ai_arranger::session {

struct SessionSnapshot {
    static constexpr uint16_t kVersion = 1;
    uint16_t version = kVersion;
    uint8_t  variation = 0;        // 0..3
    uint32_t tempo_bpm = 120;
    uint8_t  split_point = 60;
    bool     manual_bass = false;
    bool     sync_armed = false;
    uint8_t  chord_scan_mode = 0;  // app-managed index (0..3)
};

class SnapshotManager {
public:
    // Capture the reachable engine/adapter state into a snapshot.
    static SessionSnapshot capture(EngineSession& session,
                                   uint8_t chordScanModeIndex = 0) noexcept;

    // Apply a snapshot to the engine/adapter. Returns false if the snapshot is
    // invalid (and applies nothing).
    static bool restore(const SessionSnapshot& snap, EngineSession& session) noexcept;

    // Serialize to JSON.
    static std::string serialize(const SessionSnapshot& snap);

    // Parse + validate JSON. Returns false on bad version / out-of-range / parse
    // failure (out left untouched).
    static bool deserialize(const std::string& json, SessionSnapshot& out);

    // Range/version validity.
    static bool isValid(const SessionSnapshot& snap) noexcept;
};

} // namespace ai_arranger::session

#endif // AI_ARRANGER_SESSION_SNAPSHOT_MANAGER_H
