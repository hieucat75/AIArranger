// Gate 11 Task A — scaffold smoke test.
// Confirms the korg-validation library links and the claims-policy constant is
// locked to false (no hardware validation claimed).

#include "korg_validation/harness.h"
#include <cstdio>

using namespace ai_arranger::korg_validation;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

namespace ai_arranger::korg_validation { const char* harnessBanner() noexcept; }

int main() {
    std::printf("Test: Gate 11 Task A — harness scaffold\n");

    TEST("hardware_validated constant is false", kHardwareValidated == false);
    TEST("harness version is set", kHarnessVersion >= 1);
    TEST("library links (banner non-null)", harnessBanner() != nullptr);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
