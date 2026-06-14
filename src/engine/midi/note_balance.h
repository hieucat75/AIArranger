#ifndef AI_ARRANGER_MIDI_NOTE_BALANCE_H
#define AI_ARRANGER_MIDI_NOTE_BALANCE_H

#include "engine/uasf/types.h"
#include <cstdint>

namespace ai_arranger::midi {

/**
 * Stuck-note / orphan detector.
 *
 * Observes a stream of dispatched MIDI events and tracks NoteOn/NoteOff
 * balance per (channel, note). Unlike PanicHandler (a single bit per note,
 * used for the realtime all-notes-off path), NoteBalance keeps an outstanding
 * count per note so it can diagnose:
 *   - stuck notes      : a NoteOn never matched by a NoteOff
 *   - orphan note-offs : a NoteOff with no preceding NoteOn
 *   - peak polyphony   : the maximum number of simultaneously sounding notes
 *
 * Conventions:
 *   - NoteOn with velocity 0 is treated as a NoteOff (running-status idiom).
 *   - ControlChange 120 (All Sound Off) / 123 (All Notes Off) clear every
 *     active note on that channel, so a panic leaves the balance clean.
 *
 * This is a single-threaded diagnostic helper (feed it from one thread, e.g.
 * a capture sink or the CoreMidiOut dispatch tap). It does not allocate.
 */
class NoteBalance {
public:
    static constexpr uint8_t kAllSoundOff = 120;
    static constexpr uint8_t kAllNotesOff = 123;

    NoteBalance() noexcept { reset(); }

    void reset() noexcept;
    void observe(const uasf::MidiEvent& ev) noexcept;

    [[nodiscard]] int  activeCount() const noexcept { return active_total_; }
    [[nodiscard]] int  stuckNoteCount() const noexcept; // distinct (ch,note) still on
    [[nodiscard]] int  orphanOffCount() const noexcept { return orphan_off_; }
    [[nodiscard]] int  maxConcurrent() const noexcept { return max_concurrent_; }
    [[nodiscard]] int  totalNoteOn() const noexcept { return total_on_; }
    [[nodiscard]] int  totalNoteOff() const noexcept { return total_off_; }
    [[nodiscard]] bool isBalanced() const noexcept { return active_total_ == 0; }
    [[nodiscard]] bool isNoteActive(uint8_t channel, uint8_t note) const noexcept;

private:
    int16_t active_[16][128];   // outstanding NoteOn count per (channel, note)
    int     active_total_;
    int     max_concurrent_;
    int     total_on_;
    int     total_off_;
    int     orphan_off_;
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_NOTE_BALANCE_H
