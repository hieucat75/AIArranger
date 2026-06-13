#ifndef AI_ARRANGER_SFF1_MAPPER_H
#define AI_ARRANGER_SFF1_MAPPER_H

#include "importers/sff1/sff1_types.h"
#include "engine/uasf/types.h"

namespace ai_arranger::importers::sff1 {

struct SffToUasfResult {
    bool success;
    std::string error;
    uasf::StyleDefinition style;
    std::vector<std::string> warnings;
    std::vector<std::string> unmapped_features;
};

class Sff1ToUasfMapper {
public:
    SffToUasfResult map(const ParseResult& parseResult) noexcept;

private:
    uasf::SectionType mapSectionType(SffSectionType sffType) noexcept;
    uasf::TrackRole mapTrackRole(SffTrackRole sffRole) noexcept;
    uasf::MidiEvent mapMidiEvent(const SffMidiEvent& sffEv) noexcept;
};

} // namespace ai_arranger::importers::sff1
#endif // AI_ARRANGER_SFF1_MAPPER_H
