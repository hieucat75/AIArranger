#include "engine/performance/variation_manager.h"

namespace ai_arranger::performance {

VariationResolve VariationManager::onBarBoundary() noexcept {
    const int p = pending_.exchange(-1, std::memory_order_acq_rel);
    if (p < 0) return {false, current()};

    const int old = current_.exchange(p, std::memory_order_acq_rel);
    return {old != p, static_cast<Variation>(p)};
}

VariationResolve VariationManager::setImmediate(Variation v) noexcept {
    pending_.store(-1, std::memory_order_release);
    const int nv = static_cast<int>(v);
    const int old = current_.exchange(nv, std::memory_order_acq_rel);
    return {old != nv, v};
}

} // namespace ai_arranger::performance
