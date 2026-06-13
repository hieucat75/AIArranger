#include "importers/sff1/sff1_reader.h"
#include "importers/sff1/sff1_report.h"
#include "importers/sff1/sff1_mapper.h"
#include "engine/uasf/serializer.h"
#include <iostream>
#include <fstream>

using namespace ai_arranger::importers::sff1;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sff1-parser <file.sty> [--uasf-output <path>] [--json]\n";
        return 1;
    }

    std::string path = argv[1];
    bool jsonOutput = false;
    std::string uasfOutput;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--json") jsonOutput = true;
        if (arg == "--uasf-output" && i + 1 < argc) uasfOutput = argv[++i];
    }

    Sff1Reader reader;
    auto parseResult = reader.parseFile(path);

    if (jsonOutput) {
        // Simple JSON output
        std::cout << "{\n";
        std::cout << "  \"success\": " << (parseResult.success ? "true" : "false") << ",\n";
        std::cout << "  \"file\": \"" << path << "\",\n";
        std::cout << "  \"chunks\": " << parseResult.total_chunks << ",\n";
        std::cout << "  \"sections\": " << parseResult.parsed_sections << ",\n";
        std::cout << "  \"events\": " << parseResult.parsed_events << ",\n";
        std::cout << "  \"unsupported_features\": " << parseResult.unsupported_features.size() << "\n";
        std::cout << "}\n";
    } else {
        auto report = Sff1ReportGenerator::generateReport(parseResult);
        std::cout << report;
    }

    // Generate UASF candidate if requested
    if (!uasfOutput.empty() && parseResult.success) {
        Sff1ToUasfMapper mapper;
        auto uasfResult = mapper.map(parseResult);
        if (uasfResult.success) {
            // Serialize to UASF binary
            ai_arranger::uasf::UasfSerializer serializer;
            auto serialized = serializer.serialize(uasfResult.style);
            if (serialized.success) {
                std::ofstream out(uasfOutput, std::ios::binary);
                out.write(reinterpret_cast<const char*>(serialized.data.data()),
                         serialized.data.size());
                out.close();
                std::cout << "UASF output written to: " << uasfOutput
                          << " (" << serialized.data.size() << " bytes)\n";
            }
        }
    }

    return parseResult.success ? 0 : 1;
}
