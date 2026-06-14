#include "ui/theme/theme_manager.h"

namespace ai_arranger::ui {

namespace {
// Dark luxury (default): deep charcoal + warm gold accent, stage-readable text.
constexpr ThemeTokens kDarkLuxury{
    "dark-luxury", 0xFF15151A, 0xFF222229, 0xFFC8A24B, 0xFFEDEDF2, 0xFF9A9AA6,
    0xFF7AD07A, 0xFF3A3A44, 0xFFD9714B, 14.0f, 22.0f};
// Stage-readable: near-black + high-contrast white/cyan, bigger type.
constexpr ThemeTokens kStageReadable{
    "stage-readable", 0xFF000000, 0xFF101018, 0xFF18D2E6, 0xFFFFFFFF, 0xFFB8B8C8,
    0xFF35F06A, 0xFF303040, 0xFFFF5A3C, 16.0f, 26.0f};
// Minimal neon: dark slate + neon magenta/cyan accents.
constexpr ThemeTokens kMinimalNeon{
    "minimal-neon", 0xFF101014, 0xFF1A1A22, 0xFFFF3CAC, 0xFFE8E8F0, 0xFF8A8A98,
    0xFF3CFFD0, 0xFF2A2A34, 0xFFFFC24B, 14.0f, 22.0f};
} // namespace

const ThemeTokens& ThemeManager::tokensFor(int index) noexcept {
    switch (index) {
        case 1: return kStageReadable;
        case 2: return kMinimalNeon;
        default: return kDarkLuxury;
    }
}

const ThemeTokens& ThemeManager::tokens() const noexcept { return tokensFor(current_); }

bool ThemeManager::setTheme(int index) {
    if (index < 0 || index >= kThemeCount) return false;
    if (index == current_) return false;
    current_ = index;
    if (on_changed_) on_changed_();
    return true;
}

} // namespace ai_arranger::ui
