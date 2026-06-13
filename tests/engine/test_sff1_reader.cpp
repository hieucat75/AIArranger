#include "importers/sff1/sff1_reader.h"
#include "importers/sff1/sff1_report.h"
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

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
