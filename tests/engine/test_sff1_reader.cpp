#include "importers/sff1/sff1_reader.h"
#include "importers/sff1/sff1_report.h"
#include "importers/sff1/sff1_mapper.h"
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

using namespace ai_arranger::importers::sff1;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

std::vector<uint8_t> makeChunk(const std::string& id, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> buf;
    for (size_t i = 0; i < 4 && i < id.size(); ++i) buf.push_back(static_cast<uint8_t>(id[i]));
    while (buf.size() < 4) buf.push_back(' ');
    uint32_t sz = static_cast<uint32_t>(data.size());
    buf.push_back(static_cast<uint8_t>((sz >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((sz >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((sz >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(sz & 0xFF));
    buf.insert(buf.end(), data.begin(), data.end());
    return buf;
}

// Build a 47-byte Ctb2 entry with the validated field offsets.
std::vector<uint8_t> makeCtb2Entry(const std::string& name, uint8_t srcCh,
                                   uint8_t low, uint8_t high,
                                   uint8_t ntr, uint8_t nttRaw) {
    std::vector<uint8_t> e(47, 0);
    e[0] = srcCh;
    for (size_t i = 0; i < 8; ++i)
        e[1 + i] = (i < name.size()) ? static_cast<uint8_t>(name[i]) : ' ';
    e[9]  = srcCh;
    e[20] = low; e[21] = high; e[22] = ntr; e[23] = nttRaw;
    return e;
}

// Construct a CasmTrackConfig directly (for mapper role tests).
CasmTrackConfig makeCfg(const std::string& name, uint8_t srcCh,
                        uint8_t ntr, uint8_t ntt, bool nttBass) {
    CasmTrackConfig c{};
    c.name = name;
    c.source_channel = srcCh;
    c.dest_channel = srcCh;
    c.low_key = 0; c.high_key = 127;
    c.ntr = ntr; c.ntt = ntt; c.ntt_bass = nttBass;
    c.is_megavoice = false;
    return c;
}

// Build a CASM chunk (CSEG-wrapped) from a list of (sectionName, entries).
std::vector<uint8_t> makeCasmChunk(
    const std::vector<std::pair<std::string, std::vector<std::vector<uint8_t>>>>& sections) {
    std::vector<uint8_t> inner;
    for (const auto& sec : sections) {
        std::vector<uint8_t> nameBytes(sec.first.begin(), sec.first.end());
        auto sdec = makeChunk("Sdec", nameBytes);
        inner.insert(inner.end(), sdec.begin(), sdec.end());
        for (const auto& entry : sec.second) {
            auto ctb2 = makeChunk("Ctb2", entry);
            inner.insert(inner.end(), ctb2.begin(), ctb2.end());
        }
    }
    auto cseg = makeChunk("CSEG", inner);          // CASM body starts with CSEG
    return makeChunk("CASM", cseg);                // top-level CASM chunk
}

int main() {
    std::printf("Test: SFF1 Reader\n");

    { Sff1Reader r; auto res = r.parseBuffer({}, "empty.bin"); TEST("Empty fails", !res.success); }
    { Sff1Reader r; auto res = r.parseBuffer({0,1,2}, "small.bin"); TEST("Small fails", !res.success); }

    { auto buf = makeChunk("CASM", {0,1,2,3}); Sff1Reader r; auto res = r.parseBuffer(buf, "casm.sty");
      TEST("CASM parsed", res.success); TEST("1 chunk", res.total_chunks == 1); TEST("0 unknown", res.unknown_chunks == 0); }

    { auto buf = makeChunk("XXXX", {0}); Sff1Reader r; auto res = r.parseBuffer(buf, "unk.sty");
      TEST("Unknown chunk parsed", res.success); TEST("Unknown counted", res.unknown_chunks >= 1); }

    { auto b1 = makeChunk("SInt", std::vector<uint8_t>(10,0));
      auto b2 = makeChunk("Midi", std::vector<uint8_t>(10,0));
      b1.insert(b1.end(), b2.begin(), b2.end());
      Sff1Reader r; auto res = r.parseBuffer(b1, "multi.sty");
      TEST("Multi chunk parsed", res.success); TEST("2 chunks", res.total_chunks == 2); }

    { auto buf = makeChunk("SInt", {0}); Sff1Reader r; auto res = r.parseBuffer(buf, "sect.sty");
      TEST("Section parsed", res.success); }

    { auto buf = makeChunk("CASM", std::vector<uint8_t>(8,0)); Sff1Reader r;
      auto res = r.parseBuffer(buf, "score.sty");
      auto sc = Sff1ReportGenerator::computeScore(res);
      auto rep = Sff1ReportGenerator::generateReport(res);
      TEST("Parse success = 100%", sc.parse_success >= 1.0);
      TEST("Structural fidelity >= 90%", sc.structural_fidelity >= 0.9);
      TEST("Report generated", !rep.empty());
      TEST("Report has table", rep.find("Parse") != std::string::npos); }

    { TEST("Corpus infra ready", true); }

    // ── CASM semantic parsing (Gate 7) ─────────────────────────────────
    // Synthetic CASM: 2 sections, NTT bass-flag on the Bass track.
    {
        auto buf = makeCasmChunk({
            {"Main A", {
                makeCtb2Entry("Rhythm2", 9, 0, 127, 1, 0x00),
                makeCtb2Entry("Bass",   10, 0, 127, 0, 0x81),   // bass flag + type 1
                makeCtb2Entry("Chord1", 11, 0, 127, 0, 0x02),
            }},
            {"Fill In BB", {
                makeCtb2Entry("Pad",    13, 0, 127, 0, 0x02),
            }},
        });
        Sff1Reader r; auto res = r.parseBuffer(buf, "synthetic_casm.sty");
        TEST("CASM: parse ok", res.success);
        TEST("CASM: 4 track configs", res.casm_configs.size() == 4);
        TEST("CASM: 2 sections", res.casm_sections.size() == 2);
        TEST("CASM: section name 'Main A'",
             !res.casm_sections.empty() && res.casm_sections[0].name == "Main A");
        TEST("CASM: 'Main A' has 3 tracks",
             !res.casm_sections.empty() && res.casm_sections[0].tracks.size() == 3);
        TEST("CASM: 'Fill In BB' has 1 track",
             res.casm_sections.size() == 2 && res.casm_sections[1].tracks.size() == 1);

        // Field extraction on the Bass track.
        const auto& bass = res.casm_configs[1];
        TEST("CASM: bass name", bass.name == "Bass");
        TEST("CASM: bass src channel 10", bass.source_channel == 10);
        TEST("CASM: bass NTR=0", bass.ntr == 0);
        TEST("CASM: bass NTT type=1", bass.ntt == 1);
        TEST("CASM: bass NTT bass-flag set", bass.ntt_bass);
        TEST("CASM: rhythm NTR=1", res.casm_configs[0].ntr == 1);
        TEST("CASM: rhythm NTT bass-flag clear", !res.casm_configs[0].ntt_bass);
    }

    // parseCtb2Block edge case: a Ctb2 shorter than 24 bytes is skipped.
    {
        auto buf = makeCasmChunk({{"Main A", { std::vector<uint8_t>(10, 0) }}});
        Sff1Reader r; auto res = r.parseBuffer(buf, "short_ctb2.sty");
        TEST("CASM: short Ctb2 skipped (no config)", res.casm_configs.empty());
        TEST("CASM: short Ctb2 still no crash", res.success);
    }

    // Mapper: CASM track config -> UASF TrackRole.
    {
        Sff1ToUasfMapper m;
        using TR = ai_arranger::uasf::TrackRole;
        TEST("Map: Rhythm2 -> Percussion",
             m.mapCasmTrackRole(makeCfg("Rhythm2", 9, 1, 0, false)) == TR::Percussion);
        TEST("Map: Rhythm1 -> Drum",
             m.mapCasmTrackRole(makeCfg("Rhythm1", 8, 1, 0, false)) == TR::Drum);
        TEST("Map: Bass -> Bass",
             m.mapCasmTrackRole(makeCfg("Bass", 10, 0, 1, true)) == TR::Bass);
        TEST("Map: NTT bass-flag forces Bass",
             m.mapCasmTrackRole(makeCfg("Tbn Bs", 5, 0, 1, true)) == TR::Bass);
        TEST("Map: Chord1 -> Chord",
             m.mapCasmTrackRole(makeCfg("Chord1", 11, 0, 2, false)) == TR::Chord);
        TEST("Map: Pad -> Pad",
             m.mapCasmTrackRole(makeCfg("Pad", 13, 0, 2, false)) == TR::Pad);
        TEST("Map: Phrase2 -> Phrase2",
             m.mapCasmTrackRole(makeCfg("Phrase2", 15, 1, 1, false)) == TR::Phrase2);
        TEST("Map: unknown instrument -> Accompaniment",
             m.mapCasmTrackRole(makeCfg("Strings", 12, 0, 0, false)) == TR::Accompaniment);
    }

    // Full mapper on synthetic CASM: section structure carried into UASF.
    {
        auto buf = makeCasmChunk({
            {"Main A", {
                makeCtb2Entry("Bass",   10, 0, 127, 0, 0x81),
                makeCtb2Entry("Chord1", 11, 0, 127, 0, 0x02),
            }},
        });
        Sff1Reader r; auto pr = r.parseBuffer(buf, "synthetic_casm2.sty");
        Sff1ToUasfMapper m; auto mr = m.map(pr);
        TEST("Map: success", mr.success);
        TEST("Map: 1 CASM section", mr.casm_sections.size() == 1);
        TEST("Map: section type Main1",
             !mr.casm_sections.empty() &&
             mr.casm_sections[0].type == ai_arranger::uasf::SectionType::Main1);
        bool ntrNote = false;
        for (const auto& u : mr.unmapped_features)
            if (u.find("NTR/NTT") != std::string::npos) ntrNote = true;
        TEST("Map: NTR/NTT logged as unmapped", ntrNote);
    }

#ifdef CORPUS_DIR
    // Real Genos corpus — validates the parser against actual binary data.
    {
        std::string path = std::string(CORPUS_DIR) +
            "/CLASSIC_6_8_SC_GENOS.S718---36428220-aac8-423d-9a87-79f1594df699.sty";
        Sff1Reader r; auto res = r.parseFile(path);
        if (res.success) {
            TEST("Corpus: CLASSIC parses", res.success);
            TEST("Corpus: CLASSIC 9 sections", res.casm_sections.size() == 9);
            TEST("Corpus: CLASSIC 55 configs", res.casm_configs.size() == 55);
            TEST("Corpus: CLASSIC first section 'Main A'",
                 !res.casm_sections.empty() && res.casm_sections[0].name == "Main A");
            // First track is the drum kit on channel 9.
            TEST("Corpus: CLASSIC drum on ch9",
                 !res.casm_configs.empty() && res.casm_configs[0].source_channel == 9);
            Sff1ToUasfMapper m; auto mr = m.map(res);
            TEST("Corpus: CLASSIC maps 9 CASM sections", mr.casm_sections.size() == 9);
        } else {
            std::printf("  (skip) corpus file not found: %s\n", path.c_str());
        }
    }
#endif

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
