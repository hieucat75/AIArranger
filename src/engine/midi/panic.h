#ifndef AI_ARRANGER_MIDI_PANIC_H
#define AI_ARRANGER_MIDI_PANIC_H

#include "engine/uasf/types.h"
#include "engine/midi/scheduler.h"
#include <array>
#include <atomic>
#include <cstdint>

namespace ai_arranger::midi {

/**
 * Panic / All-Notes-Off handler.
 *
 * Real-time safe. Two-phase shutdown:
 * 1. Flush pending scheduler queue (silently discard)
 * 2. Send NoteOff or AllSoundOff (CC 120) for all active channels
 *
 * Architecture rule (ADR-013):
 * - No malloc/mutex/fileIO in panic path
 * - Must complete within a single audio callback
 */

class PanicHandler {
public:
    static constexpr uint8_t kAllSoundOff = 120;
    static constexpr uint8_t kAllNotesOff = 123;
    static constexpr uint8_t kMaxChannel = 15;

    PanicHandler();

    // ── Real-time safe ─────────────────────────────────────────────
    // Register a new NoteOn for tracking
    void noteOn(uint8_t channel, uint8_t note) noexcept;

    // Register NoteOff to clear tracking
    void noteOff(uint8_t channel, uint8_t note) noexcept;

    // Execute panic: clear queue + send all-notes-off to callback
    void panic(MidiScheduler& scheduler, MidiEventCallback sendCallback) noexcept;

    // Emit an explicit NoteOff (velocity 0) for every currently-active note
    // via the callback, then clear tracking. Unlike panic(), this does NOT
    // touch the scheduler queue and produces per-note events (so a NoteBalance
    // observer sees balanced on/off pairs). Used on section switch / stop to
    // avoid stuck notes. Real-time safe: bounded loop, no malloc/mutex.
    void flushActiveNotes(MidiEventCallback sendCallback) noexcept;

    // Check if any notes are still active
    [[nodiscard]] bool hasActiveNotes() const noexcept;

    // Check whether a specific (channel,note) is currently tracked as active.
    [[nodiscard]] bool isNoteActive(uint8_t channel, uint8_t note) const noexcept;

private:
    // Active note tracking: bit per channel (128 notes max per channel)
    // channel_notes[ch] has bit 'n' set if note 'n' is active
    std::array<std::atomic<uint64_t>, 16> channel_notes_low_;
    std::array<std::atomic<uint64_t>, 16> channel_notes_high_;
    std::atomic<int32_t> active_note_count_{0};
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_PANIC_H
