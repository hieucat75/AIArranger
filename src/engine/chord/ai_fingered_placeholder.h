#ifndef AI_ARRANGER_CHORD_AI_FINGERED_PLACEHOLDER_H
#define AI_ARRANGER_CHORD_AI_FINGERED_PLACEHOLDER_H

#include "engine/chord/chord_scan_mode.h"
#include "engine/chord/fingered_detector.h"

namespace ai_arranger::chord {

// AI-Fingered PLACEHOLDER — interface slot for a future ML chord model. There is
// NO model here; it deterministically falls back to the Fingered detector. The
// name makes the placeholder status explicit; no AI behaviour is claimed.
class AIFingeredPlaceholder final : public IChordScanMode {
public:
    Chord detect(const uint8_t* notes, size_t count) const noexcept override {
        return fallback_.detect(notes, count);
    }
    const char* name() const noexcept override { return "ai-fingered-placeholder"; }

private:
    FingeredDetector fallback_;
};

} // namespace ai_arranger::chord

#endif
