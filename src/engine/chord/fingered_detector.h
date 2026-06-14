#ifndef AI_ARRANGER_CHORD_FINGERED_DETECTOR_H
#define AI_ARRANGER_CHORD_FINGERED_DETECTOR_H

#include "engine/chord/chord_scan_mode.h"

namespace ai_arranger::chord {

// Fingered: the held notes spell the chord directly (triad/7th templates).
class FingeredDetector final : public IChordScanMode {
public:
    Chord detect(const uint8_t* notes, size_t count) const noexcept override;
    const char* name() const noexcept override { return "fingered"; }
};

} // namespace ai_arranger::chord

#endif
