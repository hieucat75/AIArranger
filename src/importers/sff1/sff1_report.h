#ifndef AI_ARRANGER_SFF1_REPORT_H
#define AI_ARRANGER_SFF1_REPORT_H

#include "importers/sff1/sff1_types.h"
#include <string>

namespace ai_arranger::importers::sff1 {

struct CompatibilityScore {
    double parse_success;      // 0.0 - 1.0
    double structural_fidelity; // 0.0 - 1.0
    double event_parse_rate;   // 0.0 - 1.0
    uint32_t total_chunks;
    uint32_t known_chunks;
    uint32_t unknown_chunks;
    uint32_t parsed_sections;
    uint32_t total_events;
    uint32_t megavoice_candidates;
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
