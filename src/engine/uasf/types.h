#ifndef AI_ARRANGER_UASF_TYPES_H
#define AI_ARRANGER_UASF_TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace ai_arranger::uasf {

// ── MIDI Event Types ─────────────────────────────────────────────────

enum class MidiEventType : uint8_t {
    NoteOff          = 0x80,
    NoteOn           = 0x90,
    PolyPressure     = 0xA0,
    ControlChange    = 0xB0,
    ProgramChange    = 0xC0,
    ChannelPressure  = 0xD0,
    PitchBend        = 0xE0,
};

struct MidiEvent {
    MidiEventType type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    uint64_t tick;       // Absolute tick position (0-based)
};

// ── Section Types ────────────────────────────────────────────────────

enum class SectionType : uint8_t {
    Intro1, Intro2, Intro3,
    Main1, Main2, Main3, Main4,
    Fill1, Fill2, Fill3, Fill4,
    Ending1, Ending2, Ending3,
    Break,
};

// ── Track Roles ──────────────────────────────────────────────────────

enum class TrackRole : uint8_t {
    Accompaniment,
    Chord,
    Bass,
    Drum,
    Percussion,
    Phrase1, Phrase2,
    Pad,
    Melody,
    Articulation,
};

// ── Articulation / Fidelity (ADR-013) ────────────────────────────────

enum class FidelityRequirement : uint8_t {
    Low,       // GM fallback acceptable
    Medium,    // SoundFont / AUv3 preferred
    High,      // External device or premium library required
};

enum class ArticulationProfile : uint8_t {
    Generic,
    Staccato,
    Legato,
    Mute,
    Slap,
    HammerOn,      // Guitar-specific
    PullOff,       // Guitar-specific
    MegaLike,      // Yamaha MegaVoice-style
    Tremolo,
    Trill,
    Custom,
};

struct PlaybackRecommendation {
    bool external_yamaha;
    bool external_midi;
    bool premium_library;
    bool user_soundfont;
    bool degradation_allowed;   // GM fallback only with warning
};

struct ArticulationMetadata {
    ArticulationProfile profile;
    FidelityRequirement fidelity;
    PlaybackRecommendation recommended;
    std::string vendor;          // "yamaha", "korg", etc. (informational only)
    std::string vendor_articulation;  // Original articulation name if known
    bool has_keyswitch;
    uint8_t keyswitch_note;      // 0 if no keyswitch
    std::vector<uint8_t> velocity_map; // Custom velocity mapping
};

// ── Track Definition ─────────────────────────────────────────────────

struct TrackDefinition {
    std::string name;
    uint8_t midi_channel;
    TrackRole role;
    bool is_drum;
    ArticulationMetadata articulation;
    std::vector<MidiEvent> events;
};

// ── Section Definition ───────────────────────────────────────────────

struct SectionDefinition {
    SectionType type;
    std::string name;
    uint32_t bars;
    uint32_t resolution;      // ticks per quarter note
    uint32_t beats_per_bar;
    uint32_t beat_note;       // denominator (4 = quarter note)
    std::vector<TrackDefinition> tracks;
};

// ── Style Definition ─────────────────────────────────────────────────

struct StyleDefinition {
    std::string name;
    std::string format_version;
    uint32_t tempo_bpm;
    uint32_t resolution;       // ticks per quarter note (default 480)
    std::vector<SectionDefinition> sections;
    int32_t ottava_bass;       // transpose bass tracks
    int32_t ottava_chord;      // transpose chord tracks
};

} // namespace ai_arranger::uasf

#endif // AI_ARRANGER_UASF_TYPES_H
