#ifndef AI_ARRANGER_SFF1_REPORT_H
#define AI_ARRANGER_SFF1_REPORT_H

#include "importers/sff1/sff1_types.h"
#include <string>

namespace ai_arranger::importers::sff1 {

struct CompatibilityScore {
    double parse_success;      // 0.0 - 1.0
    double structural_fidelity; // 0.0 - 1.0
    double event_parse_rate;   // 0.0 - 1.0
    double harmonic_fidelity;  // 0.0 - 1.0: CASM chord-behaviour recovery
    uint32_t total_chunks;
    uint32_t known_chunks;
    uint32_t unknown_chunks;
    uint32_t parsed_sections;
    uint32_t total_events;
    uint32_t megavoice_candidates;
    uint32_t casm_sections;    // arranger sections recovered from CASM Sdec
    uint32_t casm_tracks;      // CASM track configs (Ctb2 blocks)
    uint32_t bass_tracks;      // tracks with NTT bass-note conversion
    std::vector<std::string> unsupported;
};

class Sff1ReportGenerator {
public:
    static CompatibilityScore computeScore(const ParseResult& result) noexcept;
    static std::string generateReport(const ParseResult& result) noexcept;
    static std::string generateCompatibilityTable(const CompatibilityScore& score) noexcept;
};

} // namespace ai_arranger::importers::sff1
#endif // AI_ARRANGER_SFF1_REPORT_H
