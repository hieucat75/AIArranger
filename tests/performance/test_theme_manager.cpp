// Gate 15 Task H — theme manager (headless, ViewModel level).

#include "ui/theme/theme_manager.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace ai_arranger::ui;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

int main() {
    std::printf("Test: Gate 15 Task H — theme manager\n");

    ThemeManager tm;
    int changes = 0;
    tm.setOnChanged([&]{ ++changes; });

    // ── Default dark luxury ──
    TEST("default = dark-luxury", std::string(tm.tokens().name) == "dark-luxury" &&
         tm.current() == (int)ThemeId::DarkLuxury);
    TEST("3 themes", kThemeCount == 3);

    // ── Switch themes ──
    TEST("switch to stage-readable", tm.setTheme((int)ThemeId::StageReadable) &&
         std::string(tm.tokens().name) == "stage-readable");
    TEST("onChanged fired", changes == 1);
    TEST("switch to minimal-neon", tm.setTheme(2) &&
         std::string(tm.tokens().name) == "minimal-neon");
    TEST("re-select same -> no change", !tm.setTheme(2) && changes == 2);
    TEST("out-of-range ignored", !tm.setTheme(9) && !tm.setTheme(-1));

    // ── Tokens distinct + non-zero ──
    {
        const auto& d = ThemeManager::tokensFor(0);
        const auto& s = ThemeManager::tokensFor(1);
        TEST("themes have distinct accent", d.accent != s.accent);
        TEST("background alpha set", (d.background >> 24) == 0xFF);
    }

    // ── UI scale affects fonts ──
    {
        ThemeManager t2;
        const float base = t2.baseFont();
        t2.setUiScale(2.0f);
        TEST("scale doubles base font", near(t2.baseFont(), base * 2.0));
        TEST("title scales too", near(t2.titleFont(), t2.tokens().title_font_pt * 2.0));
        t2.setUiScale(0.1f);
        TEST("scale floored at 0.5", near(t2.uiScale(), 0.5));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
