#ifndef AI_ARRANGER_CHORD_CHORD_SCAN_MODE_H
#define AI_ARRANGER_CHORD_CHORD_SCAN_MODE_H

#include "engine/arranger/chord_input.h"
#include <cstddef>
#include <cstdint>

// ── Chord scan mode framework (Gate 12 Task E) ───────────────────────
//
// Pluggable chord detectors that map a set of held lower-zone notes to a Chord.
// Pure + realtime-safe: detect() reads a fixed caller-owned buffer, no alloc, no
// locks. Deterministic: the same held set + mode always yields the same Chord.

namespace ai_arranger::chord {

using arranger::Chord;
using arranger::ChordType;

// Strategy interface. `notes` is ascending MIDI note numbers, length `count`.
class IChordScanMode {
public:
    virtual ~IChordScanMode() = default;
    virtual Chord detect(const uint8_t* notes, size_t count) const noexcept = 0;
    virtual const char* name() const noexcept = 0;
};

// Shared template matcher: from notes (ascending), determine the chord TYPE by
// comparing the pitch-class set (relative to the lowest note) against standard
// triad/7th templates. Returns NoChord if nothing matches. `rootOut` is set to
// the lowest MIDI note.
ChordType matchChordType(const uint8_t* notes, size_t count,
                         uint8_t& rootOut) noexcept;

} // namespace ai_arranger::chord

#endif // AI_ARRANGER_CHORD_CHORD_SCAN_MODE_H
