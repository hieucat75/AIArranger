// Gate 10 Task D — CASM -> UASF semantic hardening.
//
// Audits the mapper's behaviour on malformed / incomplete CASM data. Each edge
// case has a defined contract (skip / fallback / log / fail-fast). The tests
// build synthetic ParseResults directly so they need no corpus.

#include "importers/sff1/sff1_mapper.h"
#include "importers/sff1/sff1_types.h"
#include <cstdio>
#include <string>

using namespace ai_arranger;
using namespace ai_arranger::importers::sff1;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

// ── Builders ──────────────────────────────────────────────────────────
static SffMidiEvent noteOn(uint8_t ch, uint8_t note, uint32_t tick) {
    return {tick, static_cast<uint8_t>(0x90 | (ch & 0x0F)), note, 100};
}

// A single SMF section carrying a mixed-channel event stream (as the reader
// produces). `channels` lists the MIDI channel of each event in order.
static SffSection mixedSection(const std::vector<uint8_t>& channels) {
    SffSection s{};
    s.type = SffSectionType::Main1;
    s.name = "SMF";
    s.resolution = 1920;
    SffTrack t{};
    t.role = SffTrackRole::Phrase1;
    uint32_t tick = 0;
    for (uint8_t ch : channels) t.events.push_back(noteOn(ch, 60, tick += 240));
    s.tracks.push_back(std::move(t));
    return s;
}

static CasmTrackConfig cfg(const std::string& name, uint8_t ch,
                           uint8_t ntr, uint8_t ntt, bool bass) {
    CasmTrackConfig c{};
    c.name = name;
    c.source_channel = ch;
    c.dest_channel = ch;
    c.ntr = ntr;
    c.ntt = ntt;
    c.ntt_bass = bass;
    return c;
}

// Find the UASF track on a given channel within the (single) style section.
static const uasf::TrackDefinition* trackOnChannel(const SffToUasfResult& r, uint8_t ch) {
    for (const auto& sec : r.style.sections)
        for (const auto& t : sec.tracks)
            if (t.midi_channel == ch) return &t;
    return nullptr;
}

static bool anyWarningContains(const SffToUasfResult& r, const std::string& needle) {
    for (const auto& w : r.warnings)
        if (w.find(needle) != std::string::npos) return true;
    return false;
}

int main() {
    std::printf("Test: Gate 10 Task D — CASM->UASF edge cases\n");
    Sff1ToUasfMapper mapper;

    // ── Edge 1: track with NO CASM metadata -> documented fallback ──
    {
        ParseResult pr{};
        pr.success = true;
        pr.sections.push_back(mixedSection({5, 9}));
        // No casm_configs at all.
        auto r = mapper.map(pr);
        TEST("[no-casm] mapper succeeds", r.success);
        const auto* t5 = trackOnChannel(r, 5);
        const auto* t9 = trackOnChannel(r, 9);
        TEST("[no-casm] ch5 falls back to melodic Phrase1",
             t5 && t5->role == uasf::TrackRole::Phrase1);
        TEST("[no-casm] ch9 falls back to Drum (GM convention)",
             t9 && t9->role == uasf::TrackRole::Drum);
        TEST("[no-casm] fallback is logged", anyWarningContains(r, "no CASM metadata"));
    }

    // ── Edge 2: missing NTR/NTT (zeros) -> role from name, no crash ──
    {
        ParseResult pr{};
        pr.success = true;
        pr.sections.push_back(mixedSection({10}));
        pr.casm_configs.push_back(cfg("Bass", 10, /*ntr*/0, /*ntt*/0, /*bass*/false));
        auto r = mapper.map(pr);
        const auto* t = trackOnChannel(r, 10);
        TEST("[no-ntr] role resolved from name (Bass)",
             t && t->role == uasf::TrackRole::Bass);
        TEST("[no-ntr] zero NTR/NTT carried, not invented",
             t && t->articulation.ntr == 0 && t->articulation.ntt == 0);
    }

    // ── Edge 3: bass-on flag (no 'bass' in name) -> Bass; bass-off -> not ──
    {
        ParseResult prOn{};
        prOn.success = true;
        prOn.sections.push_back(mixedSection({11}));
        prOn.casm_configs.push_back(cfg("Layer", 11, 0, 0, /*bass*/true));
        auto rOn = mapper.map(prOn);
        const auto* tOn = trackOnChannel(rOn, 11);
        TEST("[bass-on] NTT bass flag promotes track to Bass",
             tOn && tOn->role == uasf::TrackRole::Bass);

        ParseResult prOff{};
        prOff.success = true;
        prOff.sections.push_back(mixedSection({11}));
        prOff.casm_configs.push_back(cfg("Layer", 11, 0, 0, /*bass*/false));
        auto rOff = mapper.map(prOff);
        const auto* tOff = trackOnChannel(rOff, 11);
        TEST("[bass-off] no bass flag, no keyword -> not Bass",
             tOff && tOff->role != uasf::TrackRole::Bass);
    }

    // ── Edge 4: abnormal filter/config bytes -> skipped + logged ──
    {
        ParseResult pr{};
        pr.success = true;
        pr.sections.push_back(mixedSection({3}));
        // Out-of-range source channel (200) plus garbage NTR/NTT.
        pr.casm_configs.push_back(cfg("Bad", 200, 0xFF, 0x7F, false));
        auto r = mapper.map(pr);
        TEST("[abnormal] mapper still succeeds", r.success);
        TEST("[abnormal] out-of-range channel logged",
             anyWarningContains(r, "out-of-range source channel"));
        // The ch3 events still map (via fallback) — abnormal config is ignored.
        const auto* t3 = trackOnChannel(r, 3);
        TEST("[abnormal] valid events still mapped via fallback", t3 != nullptr);
    }

    // ── Edge 5: Sdec section overlap (duplicate names) -> both kept ──
    {
        ParseResult pr{};
        pr.success = true;
        pr.sections.push_back(mixedSection({8}));
        CasmSection a; a.name = "Main A"; a.tracks.push_back(cfg("Rhythm1", 8, 0, 0, false));
        CasmSection b; b.name = "Main A"; b.tracks.push_back(cfg("Bass", 10, 0, 0, false));
        pr.casm_sections.push_back(a);
        pr.casm_sections.push_back(b);
        auto r = mapper.map(pr);
        TEST("[overlap] both duplicate-named CASM sections preserved",
             r.casm_sections.size() == 2);
        TEST("[overlap] both map to the same section type (Main1)",
             r.casm_sections.size() == 2 &&
             r.casm_sections[0].type == r.casm_sections[1].type);
    }

    // ── Edge 6: totally empty parse result -> fail-fast ──
    {
        ParseResult pr{};
        pr.success = true; // parse ok, but nothing to map
        auto r = mapper.map(pr);
        TEST("[empty] no sections -> fail-fast with error",
             !r.success && !r.error.empty());
    }

    // ── Edge 7: CASM present but the SMF stream is empty -> structure only ──
    {
        ParseResult pr{};
        pr.success = true;
        CasmSection a; a.name = "Intro A"; a.tracks.push_back(cfg("Bass", 10, 0, 0, false));
        pr.casm_sections.push_back(a);
        auto r = mapper.map(pr);
        TEST("[casm-only] succeeds on CASM structure with no events", r.success);
        TEST("[casm-only] CASM section structure exposed",
             r.casm_sections.size() == 1);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
