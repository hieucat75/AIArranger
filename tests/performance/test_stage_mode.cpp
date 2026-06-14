// Gate 15 Task I — performer layout state (headless).

#include "ui/performance/stage_mode.h"
#include <cstdio>

using namespace ai_arranger::ui;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 15 Task I — stage mode\n");

    StageMode s;
    int changes = 0;
    s.setOnChanged([&]{ ++changes; });

    TEST("default Desktop", s.mode() == LayoutMode::Desktop && !s.isStage());
    TEST("default: small transport", !s.largeTransport());

    s.toggle();
    TEST("toggle -> Stage", s.isStage() && changes == 1);
    TEST("stage: large transport", s.largeTransport());
    TEST("stage hides diagnostics", !s.showsDiagnostics());

    // Drawer only matters in Desktop.
    s.setDiagnosticsDrawer(true);
    TEST("drawer open but stage -> still hidden", !s.showsDiagnostics());
    s.toggle();
    TEST("back to Desktop", !s.isStage());
    TEST("desktop + drawer -> diagnostics shown", s.showsDiagnostics());

    int before = changes;
    s.setMode(LayoutMode::Desktop);
    TEST("set same mode -> no change", changes == before);
    s.setDiagnosticsDrawer(false);
    TEST("drawer closed -> diagnostics hidden", !s.showsDiagnostics());

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
