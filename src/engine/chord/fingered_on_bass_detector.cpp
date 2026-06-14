#include "engine/chord/fingered_on_bass_detector.h"

namespace ai_arranger::chord {

Chord FingeredOnBassDetector::detect(const uint8_t* notes, size_t count) const noexcept {
    Chord c{ChordType::NoChord, 0, 0, 0};
    if (count == 0) return c;

    const uint8_t bass = notes[0];

    // Detect the chord from the notes ABOVE the bass (slash chord). Need at
    // least a triad up there.
    if (count >= 4) {
        uint8_t root = 0;
        ChordType t = matchChordType(notes + 1, count - 1, root);
        if (t != ChordType::NoChord) {
            c.type = t; c.root = root; c.bass = bass;
            return c;
        }
    }

    // Fallback: detect over the whole set, keep the lowest as bass.
    uint8_t root = 0;
    c.type = matchChordType(notes, count, root);
    c.root = root;
    c.bass = bass;
    return c;
}

} // namespace ai_arranger::chord
