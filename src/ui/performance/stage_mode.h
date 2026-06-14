#ifndef AI_ARRANGER_UI_PERFORMANCE_STAGE_MODE_H
#define AI_ARRANGER_UI_PERFORMANCE_STAGE_MODE_H

#include <functional>

// ── Performer layout state (Gate 15 Task I) ──────────────────────────
//
// UI-agnostic layout state machine for the performer redesign: Desktop (panels +
// optional diagnostics drawer) vs Stage (large transport, minimal chrome,
// fullscreen). Stage mode hides diagnostics by design. Headless + testable.

namespace ai_arranger::ui {

enum class LayoutMode { Desktop, Stage };

class StageMode {
public:
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    LayoutMode mode() const noexcept { return mode_; }
    bool isStage() const noexcept { return mode_ == LayoutMode::Stage; }

    void setMode(LayoutMode m) noexcept {
        if (m == mode_) return;
        mode_ = m;
        if (on_changed_) on_changed_();
    }
    void toggle() noexcept {
        setMode(mode_ == LayoutMode::Desktop ? LayoutMode::Stage : LayoutMode::Desktop);
    }

    // Diagnostics drawer is a Desktop-only affordance.
    void setDiagnosticsDrawer(bool open) noexcept {
        if (open == drawer_) return;
        drawer_ = open;
        if (on_changed_) on_changed_();
    }
    bool diagnosticsDrawerOpen() const noexcept { return drawer_; }

    // Whether diagnostics are visible given the current mode + drawer.
    bool showsDiagnostics() const noexcept {
        return mode_ == LayoutMode::Desktop && drawer_;
    }
    // Stage mode uses oversized, finger-friendly transport controls.
    bool largeTransport() const noexcept { return mode_ == LayoutMode::Stage; }

private:
    LayoutMode mode_ = LayoutMode::Desktop;
    bool drawer_ = false;
    std::function<void()> on_changed_;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_PERFORMANCE_STAGE_MODE_H
