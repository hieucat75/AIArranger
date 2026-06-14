#ifndef AI_ARRANGER_UI_THEME_THEME_MANAGER_H
#define AI_ARRANGER_UI_THEME_THEME_MANAGER_H

#include <cstdint>
#include <functional>

// ── Theme system (Gate 15 Task H) ────────────────────────────────────
//
// UI-toolkit-agnostic theme tokens (ARGB colors + typography) and selection. The
// JUCE LookAndFeel (apps/macos) maps these tokens to components; this layer is
// headless + CI-tested. Includes a UI-scale hook for iPad/Retina sizing.

namespace ai_arranger::ui {

struct ThemeTokens {
    const char* name = "";
    uint32_t background = 0;   // ARGB
    uint32_t surface = 0;
    uint32_t accent = 0;
    uint32_t text = 0;
    uint32_t text_dim = 0;
    uint32_t indicator_on = 0;
    uint32_t indicator_off = 0;
    uint32_t warning = 0;
    float    base_font_pt = 14.0f;
    float    title_font_pt = 22.0f;
};

enum class ThemeId : int { DarkLuxury = 0, StageReadable = 1, MinimalNeon = 2 };
constexpr int kThemeCount = 3;

class ThemeManager {
public:
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    // Select by index (0..kThemeCount-1). Out-of-range ignored. Returns true if
    // the theme changed.
    bool setTheme(int index);
    int  current() const noexcept { return current_; }

    // UI scale (1.0 desktop; >1 for iPad/large displays). Affects font getters.
    void setUiScale(float s) noexcept { scale_ = s < 0.5f ? 0.5f : s; }
    float uiScale() const noexcept { return scale_; }

    const ThemeTokens& tokens() const noexcept;
    float baseFont() const noexcept { return tokens().base_font_pt * scale_; }
    float titleFont() const noexcept { return tokens().title_font_pt * scale_; }

    static const ThemeTokens& tokensFor(int index) noexcept;

private:
    int current_ = static_cast<int>(ThemeId::DarkLuxury);
    float scale_ = 1.0f;
    std::function<void()> on_changed_;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_THEME_THEME_MANAGER_H
