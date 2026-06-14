#ifndef AI_ARRANGER_CHORD_FINGERED_ON_BASS_DETECTOR_H
#define AI_ARRANGER_CHORD_FINGERED_ON_BASS_DETECTOR_H

#include "engine/chord/chord_scan_mode.h"

namespace ai_arranger::chord {

// Fingered-on-bass (slash chords): the lowest note is the bass; the chord is
// detected from the notes above it. Falls back to whole-set detection.
class FingeredOnBassDetector final : public IChordScanMode {
public:
    Chord detect(const uint8_t* notes, size_t count) const noexcept override;
    const char* name() const noexcept override { return "fingered-on-bass"; }
};

} // namespace ai_arranger::chord

#endif
