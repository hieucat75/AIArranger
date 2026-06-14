#include "korg_validation/harness.h"

// Translation unit anchor for the korg-validation library. Component
// implementations (capture, analyzers, reporter) are added in later tasks.

namespace ai_arranger::korg_validation {

const char* harnessBanner() noexcept {
    return "korg-validation harness (synthetic-only; hardware_validated=false)";
}

} // namespace ai_arranger::korg_validation
