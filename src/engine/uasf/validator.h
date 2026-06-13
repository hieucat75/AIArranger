#ifndef AI_ARRANGER_UASF_VALIDATOR_H
#define AI_ARRANGER_UASF_VALIDATOR_H

#include "engine/uasf/types.h"
#include <string>
#include <vector>

namespace ai_arranger::uasf {

struct ValidationIssue {
    enum Severity { INFO, WARNING, ERROR };
    Severity severity;
    std::string message;
    std::string location; // e.g., "Section 2 / Track 1"
};

struct ValidationResult {
    bool valid;
    std::vector<ValidationIssue> issues;
};

class UasfValidator {
public:
    ValidationResult validate(const StyleDefinition& style) noexcept;
    ValidationResult validateBinary(const std::vector<uint8_t>& data) noexcept;

private:
    void addIssue(ValidationIssue::Severity sev, const std::string& msg,
                  const std::string& loc = "") noexcept;

    std::vector<ValidationIssue> issues_;
};

} // namespace ai_arranger::uasf
#endif // AI_ARRANGER_UASF_VALIDATOR_H
