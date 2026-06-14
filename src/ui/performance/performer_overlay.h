#ifndef AI_ARRANGER_UI_PERFORMANCE_PERFORMER_OVERLAY_H
#define AI_ARRANGER_UI_PERFORMANCE_PERFORMER_OVERLAY_H

#include <cstdint>
#include <functional>

// ── Realtime visual feedback (Gate 15 Task J) ────────────────────────
//
// UI-agnostic aggregation of the 7 performer indicators: fill queued, variation
// pending, sync armed, MIDI output active, latency level, reconnecting, current
// groove profile. Fed from telemetry/other ViewModels; the JUCE overlay renders
// it. onChanged fires only on an actual change.

namespace ai_arranger::ui {

enum class LatencyLevel { Good, Warn, Bad };

class PerformerOverlay {
public:
    // Latency thresholds (ms).
    static constexpr double kWarnMs = 10.0;
    static constexpr double kBadMs  = 30.0;

    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    void setFillQueued(bool v)        { set(fill_queued_, v); }
    void setVariationPending(bool v)  { set(variation_pending_, v); }
    void setSyncArmed(bool v)         { set(sync_armed_, v); }
    void setMidiActive(bool v)        { set(midi_active_, v); }
    void setReconnecting(bool v)      { set(reconnecting_, v); }
    void setGrooveProfile(uint8_t i)  { set(groove_profile_, i); }
    void setLatencyMs(double ms) {
        const LatencyLevel l = ms >= kBadMs ? LatencyLevel::Bad
                             : ms >= kWarnMs ? LatencyLevel::Warn : LatencyLevel::Good;
        set(latency_, l);
    }

    bool fillQueued() const noexcept { return fill_queued_; }
    bool variationPending() const noexcept { return variation_pending_; }
    bool syncArmed() const noexcept { return sync_armed_; }
    bool midiActive() const noexcept { return midi_active_; }
    bool reconnecting() const noexcept { return reconnecting_; }
    uint8_t grooveProfile() const noexcept { return groove_profile_; }
    LatencyLevel latency() const noexcept { return latency_; }

    static constexpr int kIndicatorCount = 7;

private:
    template <typename T> void set(T& field, T v) {
        if (field == v) return;
        field = v;
        if (on_changed_) on_changed_();
    }
    bool fill_queued_ = false;
    bool variation_pending_ = false;
    bool sync_armed_ = false;
    bool midi_active_ = false;
    bool reconnecting_ = false;
    uint8_t groove_profile_ = 0;
    LatencyLevel latency_ = LatencyLevel::Good;
    std::function<void()> on_changed_;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_PERFORMANCE_PERFORMER_OVERLAY_H
