#ifndef AI_ARRANGER_TRACE_TRACE_RECORD_H
#define AI_ARRANGER_TRACE_TRACE_RECORD_H

#include <cstdint>
#include <type_traits>

// ── Fixed-size latency trace record ──────────────────────────────────
//
// One POD row written by an RT trace point into the lock-free ring. Every field
// is a small integer / enum id — NO std::string, NO pointer, NO owning type — so
// the record is trivially copyable and a raw memcpy across the ring is safe. All
// string-ish dimensions (device, section, chord, lifecycle) are encoded as small
// ids; the report tool maps ids back to human labels off the RT path.
//
// Timestamps are monotonic nanoseconds from trace::traceNowNs(). Correlation id
// links the five stages of a single input→output journey (see TraceStage).

namespace ai_arranger::trace {

// The five mandated latency waypoints, in causal order. Kept <256 so it fits a
// single byte in the record.
enum class TraceStage : uint8_t {
    InputNoteOn       = 0, // CoreMIDI read thread received a NoteOn
    ChordConfirmed    = 1, // chord confirmed after latch/hold, handed to arranger
    AccompFirstEvent  = 2, // scheduler produced the FIRST accompaniment event
    OutputEnqueue     = 3, // event pushed onto the MIDI output queue
    MidiSend          = 4, // CoreMIDI MIDISend actually invoked
    Lifecycle         = 5, // non-waypoint marker (start/stop/panic/hotplug/...)
};

// Optional lifecycle context stamped on a record. kNone for a plain latency
// waypoint; the others let the report tool bucket "before/after hot-plug",
// "after stop/panic/switch/close" without a second data source.
enum class LifecycleTag : uint8_t {
    kNone          = 0,
    kStart         = 1,
    kStop          = 2,
    kPanic         = 3,
    kSectionSwitch = 4,
    kChordChange   = 5,
    kHotplugAdd    = 6, // device added / reconnected
    kHotplugRemove = 7, // device removed / disconnected
    kAppClose      = 8,
    kQueueOverflow = 9, // an output/scheduler queue rejected an event
    kLateTick      = 10, // tick fell behind and had to catch up
};

#pragma pack(push, 1)
struct TraceRecord {
    uint64_t timestamp_ns;     // trace::traceNowNs() at the waypoint
    uint64_t correlation_id;   // links the 5 stages of one input→output journey
    uint16_t tempo_bpm;        // current tempo (whole bpm; 0 if unknown)
    uint16_t input_device_id;  // small id for the input endpoint (0 = none)
    uint16_t output_device_id; // small id for the output endpoint (0 = none)
    uint8_t  stage;            // TraceStage
    uint8_t  channel;          // MIDI channel 0-15 (0xFF = n/a)
    uint8_t  note;             // MIDI note 0-127  (0xFF = n/a)
    uint8_t  velocity;         // MIDI velocity 0-127 (0xFF = n/a)
    uint8_t  chord_root;       // 0-127 (0xFF = n/a)
    uint8_t  chord_type;       // arranger::ChordType id (0xFF = n/a)
    uint8_t  section;          // section index/id (0xFF = n/a)
    uint8_t  lifecycle_tag;    // LifecycleTag
};
#pragma pack(pop)

static_assert(std::is_trivially_copyable<TraceRecord>::value,
              "TraceRecord must be trivially copyable for lock-free ring memcpy");
static_assert(sizeof(TraceRecord) == 30, "TraceRecord layout drifted (expected 30 bytes)");

inline constexpr uint8_t kNa = 0xFF; // sentinel for "field not applicable"

// Stable stage names for the report tool / CSV header. Index by TraceStage.
inline const char* stageName(uint8_t stage) noexcept {
    switch (static_cast<TraceStage>(stage)) {
        case TraceStage::InputNoteOn:      return "input_note_on";
        case TraceStage::ChordConfirmed:   return "chord_confirmed";
        case TraceStage::AccompFirstEvent: return "accomp_first_event";
        case TraceStage::OutputEnqueue:    return "output_enqueue";
        case TraceStage::MidiSend:         return "midi_send";
        case TraceStage::Lifecycle:        return "lifecycle";
    }
    return "unknown";
}

inline const char* lifecycleName(uint8_t tag) noexcept {
    switch (static_cast<LifecycleTag>(tag)) {
        case LifecycleTag::kNone:          return "none";
        case LifecycleTag::kStart:         return "start";
        case LifecycleTag::kStop:          return "stop";
        case LifecycleTag::kPanic:         return "panic";
        case LifecycleTag::kSectionSwitch: return "section_switch";
        case LifecycleTag::kChordChange:   return "chord_change";
        case LifecycleTag::kHotplugAdd:    return "hotplug_add";
        case LifecycleTag::kHotplugRemove: return "hotplug_remove";
        case LifecycleTag::kAppClose:      return "app_close";
        case LifecycleTag::kQueueOverflow: return "queue_overflow";
        case LifecycleTag::kLateTick:      return "late_tick";
    }
    return "unknown";
}

} // namespace ai_arranger::trace

#endif // AI_ARRANGER_TRACE_TRACE_RECORD_H
