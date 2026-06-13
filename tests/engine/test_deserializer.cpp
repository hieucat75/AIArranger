#include "engine/uasf/format.h"
#include "engine/uasf/deserializer.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace ai_arranger::uasf;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: UASF Deserializer (error handling)\n");
    UasfDeserializer deserializer;

    // ── Empty data ─────────────────────────────────────────────────
    auto r1 = deserializer.deserialize({});
    TEST("Empty data returns error", !r1.success);
    TEST("Empty error is descriptive", !r1.error.empty());

    // ── Invalid magic ──────────────────────────────────────────────
    std::vector<uint8_t> badMagic(sizeof(format::FileHeader), 0xFF);
    auto r2 = deserializer.deserialize(badMagic);
    TEST("Bad magic returns error", !r2.success);

    // ── Wrong version ──────────────────────────────────────────────
    format::FileHeader wrongVer;
    std::memset(&wrongVer, 0, sizeof(wrongVer));
    wrongVer.magic = format::kMagic;
    wrongVer.version = 999;
    std::vector<uint8_t> wrongVerData(sizeof(wrongVer));
    std::memcpy(wrongVerData.data(), &wrongVer, sizeof(wrongVer));
    auto r3 = deserializer.deserialize(wrongVerData);
    TEST("Wrong version returns error", !r3.success);

    // ── Truncated data ─────────────────────────────────────────────
    // Small buffer with just magic
    auto hdr = wrongVerData;
    hdr.resize(sizeof(format::FileHeader));
    format::FileHeader* fh = reinterpret_cast<format::FileHeader*>(hdr.data());
    fh->magic = format::kMagic;
    fh->version = format::kVersion;
    fh->section_count = 10; // Claims 10 sections but no data
    auto r4 = deserializer.deserialize(hdr);
    TEST("Truncated sections returns error", !r4.success);

    // ── Large section count ────────────────────────────────────────
    fh->section_count = 100;
    auto r5 = deserializer.deserialize(hdr);
    TEST("Large section count returns error", !r5.success);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
