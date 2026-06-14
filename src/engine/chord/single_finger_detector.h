#ifndef AI_ARRANGER_CHORD_SINGLE_FINGER_DETECTOR_H
#define AI_ARRANGER_CHORD_SINGLE_FINGER_DETECTOR_H

#include "engine/chord/chord_scan_mode.h"

namespace ai_arranger::chord {

// Single-finger (simplified, documented convention):
//   1 note            -> Major (root = that note)
//   root + minor 3rd  -> Minor
//   root + minor 7th  -> Dom7
//   root + 5th        -> Power
//   3+ notes          -> delegate to the fingered templates
// Deterministic; not a Yamaha/Casio-specific spatial scheme.
class SingleFingerDetector final : public IChordScanMode {
public:
    Chord detect(const uint8_t* notes, size_t count) const noexcept override;
    const char* name() const noexcept override { return "single-finger"; }
};

} // namespace ai_arranger::chord

#endif
