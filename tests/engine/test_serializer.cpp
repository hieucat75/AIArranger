#include "engine/uasf/format.h"
#include "engine/uasf/serializer.h"
#include "engine/uasf/deserializer.h"
#include "engine/arranger/style_player.h"
#include <cstdio>
#include <cstring>

using namespace ai_arranger::uasf;
using namespace ai_arranger::arranger;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: UASF Serializer\n");

    // ── Serialize demo style ───────────────────────────────────────
    auto style = buildDemoStyle();
    style.name = "Gate 3 Test Style";
    style.tempo_bpm = 120;
    style.resolution = 480;

    UasfSerializer serializer;
    auto result = serializer.serialize(style);

    TEST("Serialize succeeds", result.success);
    TEST("Data produced", result.data.size() > sizeof(format::FileHeader));
    TEST("Data size < 10 MB", result.data.size() < 10 * 1024 * 1024);

    // Check magic
    format::FileHeader header;
    std::memcpy(&header, result.data.data(), sizeof(header));
    TEST("Magic is UASF", header.magic == format::kMagic);
    TEST("Version is 1", header.version == format::kVersion);
    TEST("Section count matches", header.section_count == style.sections.size());

    // ── Roundtrip ──────────────────────────────────────────────────
    UasfDeserializer deserializer;
    auto rt = deserializer.deserialize(result.data);

    TEST("Roundtrip succeeds", rt.success);
    TEST("Format version preserved", rt.style.format_version == "1.0");
    TEST("Section count preserved (serializer)", rt.style.sections.size() == style.sections.size());

    if (rt.success && rt.style.sections.size() == style.sections.size()) {
        for (size_t s = 0; s < style.sections.size(); ++s) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Section %zu bars preserved", s);
            TEST(buf, rt.style.sections[s].bars == style.sections[s].bars);

            std::snprintf(buf, sizeof(buf), "Section %zu resolution preserved", s);
            TEST(buf, rt.style.sections[s].resolution == style.sections[s].resolution);

            std::snprintf(buf, sizeof(buf), "Section %zu beats_per_bar preserved", s);
            TEST(buf, rt.style.sections[s].beats_per_bar == style.sections[s].beats_per_bar);
        }
    }

    // ── Verify track data preserved ─────────────────────────────────
    if (rt.success && rt.style.sections.size() > 0) {
        const auto& origTrack = style.sections[0].tracks[0];
        const auto& deserTrack = rt.style.sections[0].tracks[0];

        TEST("Track role preserved", deserTrack.role == origTrack.role);
        TEST("Track channel preserved", deserTrack.midi_channel == origTrack.midi_channel);
        TEST("Track drum flag preserved", deserTrack.is_drum == origTrack.is_drum);
    }

    // ── Verify event count preserved ────────────────────────────────
    if (rt.success) {
        size_t origEvents = 0;
        size_t descEvents = 0;
        for (const auto& s : style.sections) {
            for (const auto& t : s.tracks) origEvents += t.events.size();
        }
        for (const auto& s : rt.style.sections) {
            for (const auto& t : s.tracks) descEvents += t.events.size();
        }
        TEST("Total event count preserved", origEvents == descEvents);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
