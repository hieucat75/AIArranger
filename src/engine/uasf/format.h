#ifndef AI_ARRANGER_UASF_FORMAT_H
#define AI_ARRANGER_UASF_FORMAT_H

#include <cstdint>
#include <cstddef>

namespace ai_arranger::uasf::format {

// ── Binary format constants ────────────────────────────────────────

constexpr uint32_t kMagic = 0x46534155; // "UASF" little-endian
constexpr uint32_t kVersion = 1;
constexpr uint32_t kMaxSections = 32;
constexpr uint32_t kMaxTracksPerSection = 16;
constexpr uint32_t kMaxTrackNameLength = 64;
constexpr uint32_t kMaxEvents = 65536;

// ── File header (16 bytes) ─────────────────────────────────────────

#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic;           // kMagic
    uint32_t version;         // kVersion
    uint32_t flags;           // reserved
    uint32_t section_count;   // number of sections
};

// ── Section header ─────────────────────────────────────────────────

struct SectionHeader {
    uint8_t  type;            // SectionType enum
    uint16_t bars;
    uint16_t resolution;      // ticks per quarter note
    uint8_t  beats_per_bar;
    uint8_t  beat_note;
    uint8_t  track_count;
    uint8_t  padding;         // reserved
};

// ── Track header (variable-length due to name) ─────────────────────

struct TrackHeader {
    uint8_t  name_length;     // bytes in name (0-63)
    uint8_t  midi_channel;
    uint8_t  role;            // TrackRole enum
    uint8_t  is_drum;
    uint8_t  articulation_profile;  // ArticulationProfile enum
    uint8_t  fidelity;             // FidelityRequirement enum
    uint16_t playback_flags;       // PlaybackRecommendation bitmask
    uint32_t event_count;
    // Followed by: char name[name_length]
    // Followed by: MidiEventSerialized[event_count]
};

// ── MIDI event (8 bytes, delta-encoded ticks) ──────────────────────

struct MidiEventSerialized {
    uint32_t tick_delta;      // ticks from previous event
    uint8_t  type_channel;    // high nibble = type, low nibble = channel
    uint8_t  data1;
    uint8_t  data2;
    uint8_t  padding;
};
#pragma pack(pop)

// ── Validation constants ───────────────────────────────────────────

constexpr uint32_t kMaxFileSize = 10 * 1024 * 1024; // 10 MB
constexpr uint32_t kMinFileSize = sizeof(FileHeader);

// ── Playback recommendation bitmask ────────────────────────────────

enum PlaybackFlag : uint16_t {
    PF_EXTERNAL_YAMAHA    = 1 << 0,
    PF_EXTERNAL_MIDI      = 1 << 1,
    PF_PREMIUM_LIBRARY    = 1 << 2,
    PF_USER_SOUNDFONT     = 1 << 3,
    PF_DEGRADATION_OK     = 1 << 4,
};

} // namespace ai_arranger::uasf::format
#endif // AI_ARRANGER_UASF_FORMAT_H
