#include "importers/sff1/sff1_mapper.h"

namespace ai_arranger::importers::sff1 {

SffToUasfResult Sff1ToUasfMapper::map(const ParseResult& parseResult) noexcept {
    SffToUasfResult result;
    result.success = false;

    if (!parseResult.success || parseResult.sections.empty()) {
        result.error = "No sections to map";
        return result;
    }

    auto& style = result.style;
    style.name = "Imported from " + parseResult.file_path;
    style.format_version = "1.0";
    style.tempo_bpm = 120;
    style.resolution = 480;

    for (const auto& sffSection : parseResult.sections) {
        uasf::SectionDefinition section;
        section.type = mapSectionType(sffSection.type);
        section.name = sffSection.name;
        section.bars = sffSection.bars;
        section.resolution = sffSection.resolution;
        section.beats_per_bar = 4;
        section.beat_note = 4;

        for (const auto& sffTrack : sffSection.tracks) {
            uasf::TrackDefinition track;
            track.name = "Track " + std::to_string(static_cast<int>(track.role));
            track.midi_channel = sffTrack.midi_channel;
            track.role = mapTrackRole(sffTrack.role);
            track.is_drum = (sffTrack.role == SffTrackRole::Rhythm1 ||
                            sffTrack.role == SffTrackRole::Rhythm2);

            // Articulation metadata (ADR-013)
            track.articulation.profile = uasf::ArticulationProfile::Generic;
            track.articulation.fidelity = uasf::FidelityRequirement::High;
            if (track.role == uasf::TrackRole::Bass ||
                track.role == uasf::TrackRole::Percussion) {
                track.articulation.fidelity = uasf::FidelityRequirement::Medium;
            }

            // Map MIDI events
            for (const auto& sffEv : sffTrack.events) {
                track.events.push_back(mapMidiEvent(sffEv));
            }

            section.tracks.push_back(std::move(track));
        }

        style.sections.push_back(std::move(section));
    }

    // Check for unmapped features
    if (!parseResult.unsupported_features.empty()) {
        result.unmapped_features = parseResult.unsupported_features;
    }

    result.success = true;
    return result;
}

uasf::SectionType Sff1ToUasfMapper::mapSectionType(SffSectionType sffType) noexcept {
    switch (sffType) {
        case SffSectionType::Intro1:  return uasf::SectionType::Intro1;
        case SffSectionType::Intro2:  return uasf::SectionType::Intro2;
        case SffSectionType::Intro3:  return uasf::SectionType::Intro3;
        case SffSectionType::Main1:   return uasf::SectionType::Main1;
        case SffSectionType::Main2:   return uasf::SectionType::Main2;
        case SffSectionType::Main3:   return uasf::SectionType::Main3;
        case SffSectionType::Main4:   return uasf::SectionType::Main4;
        case SffSectionType::Fill1:   return uasf::SectionType::Fill1;
        case SffSectionType::Fill2:   return uasf::SectionType::Fill2;
        case SffSectionType::Fill3:   return uasf::SectionType::Fill3;
        case SffSectionType::Fill4:   return uasf::SectionType::Fill4;
        case SffSectionType::Ending1: return uasf::SectionType::Ending1;
        case SffSectionType::Ending2: return uasf::SectionType::Ending2;
        case SffSectionType::Ending3: return uasf::SectionType::Ending3;
        case SffSectionType::Break:   return uasf::SectionType::Break;
        default: return uasf::SectionType::Main1;
    }
}

uasf::TrackRole Sff1ToUasfMapper::mapTrackRole(SffTrackRole sffRole) noexcept {
    switch (sffRole) {
        case SffTrackRole::Rhythm1:  return uasf::TrackRole::Drum;
        case SffTrackRole::Rhythm2:  return uasf::TrackRole::Percussion;
        case SffTrackRole::Bass:     return uasf::TrackRole::Bass;
        case SffTrackRole::Chord1:   return uasf::TrackRole::Chord;
        case SffTrackRole::Chord2:   return uasf::TrackRole::Chord;
        case SffTrackRole::Pad:      return uasf::TrackRole::Pad;
        case SffTrackRole::Phrase1:  return uasf::TrackRole::Phrase1;
        case SffTrackRole::Phrase2:  return uasf::TrackRole::Phrase2;
        default:                     return uasf::TrackRole::Accompaniment;
    }
}

uasf::MidiEvent Sff1ToUasfMapper::mapMidiEvent(const SffMidiEvent& sffEv) noexcept {
    uasf::MidiEvent ev;
    ev.type = static_cast<uasf::MidiEventType>(sffEv.status & 0xF0);
    ev.channel = sffEv.status & 0x0F;
    ev.data1 = sffEv.data1;
    ev.data2 = sffEv.data2;
    ev.tick = sffEv.tick;
    return ev;
}

} // namespace ai_arranger::importers::sff1
