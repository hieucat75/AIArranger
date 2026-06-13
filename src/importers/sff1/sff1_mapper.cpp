#include "importers/sff1/sff1_mapper.h"
#include <algorithm>
#include <cctype>

namespace ai_arranger::importers::sff1 {

SffToUasfResult Sff1ToUasfMapper::map(const ParseResult& parseResult) noexcept {
    SffToUasfResult result;
    result.success = false;

    if (!parseResult.success ||
        (parseResult.sections.empty() && parseResult.casm_sections.empty())) {
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

    // ── CASM-derived section structure (Task D) ────────────────────────
    // Build UASF sections from the CASM Sdec/Ctb2 data: real section names,
    // per-track roles and channels. Events are NOT attached here — they
    // remain in the single SMF MTrk (per-section event splitting is Gate 8+).
    for (const auto& cs : parseResult.casm_sections) {
        uasf::SectionDefinition sec;
        sec.type = mapCasmSectionType(cs.name);
        sec.name = cs.name;
        sec.bars = 0;
        sec.resolution = style.resolution;
        sec.beats_per_bar = 4;
        sec.beat_note = 4;

        for (const auto& t : cs.tracks) {
            uasf::TrackDefinition track;
            track.name = t.name;
            track.midi_channel = t.source_channel;
            track.role = mapCasmTrackRole(t);
            track.is_drum = (track.role == uasf::TrackRole::Drum ||
                             track.role == uasf::TrackRole::Percussion);
            track.articulation.profile = uasf::ArticulationProfile::Generic;
            track.articulation.fidelity = uasf::FidelityRequirement::High;

            // Pass NTR/NTT from CASM config to UASF articulation
            for (const auto& cfg : parseResult.casm_configs) {
                if (track.name.find(cfg.name) != std::string::npos) {
                    track.articulation.ntr = cfg.ntr;
                    track.articulation.ntt = cfg.ntt;
                    break;
                }
            }

            sec.tracks.push_back(std::move(track));
        }
        result.casm_sections.push_back(std::move(sec));
    }

    // Check for unmapped features
    if (!parseResult.unsupported_features.empty()) {
        result.unmapped_features = parseResult.unsupported_features;
    }

    // NTR/NTT values passed to UASF articulation metadata (Gate 8)
    if (!parseResult.casm_configs.empty()) {
        // Count mapped tracks
        size_t mappedNtr = 0;
        for (const auto& sec : result.casm_sections) {
            for (const auto& trk : sec.tracks) {
                if (trk.articulation.ntr != 0) mappedNtr++;
            }
        }
        result.warnings.push_back(
            "NTR/NTT mapped for " + std::to_string(mappedNtr) +
            " tracks (supports Root, Fifth, Chord, Bass, Fixed)");
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

uasf::TrackRole Sff1ToUasfMapper::mapCasmTrackRole(const CasmTrackConfig& cfg) noexcept {
    std::string n;
    n.reserve(cfg.name.size());
    for (char c : cfg.name)
        n.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    auto has = [&](const char* s) { return n.find(s) != std::string::npos; };

    // Drums live on the GM percussion channel (0-based 9) regardless of name.
    if (cfg.source_channel == 9 || has("rhythm") || has("drum")) {
        // Rhythm2 = percussion overlay, Rhythm1 / main kit = drums
        // (consistent with mapTrackRole's SffTrackRole convention).
        if (has("rhythm2")) return uasf::TrackRole::Percussion;
        return uasf::TrackRole::Drum;
    }
    // Bass: explicit name or NTT bass-note conversion flag.
    if (has("bass") || cfg.ntt_bass) return uasf::TrackRole::Bass;
    if (has("chord"))                return uasf::TrackRole::Chord;
    if (has("pad"))                  return uasf::TrackRole::Pad;
    if (has("phrase2"))              return uasf::TrackRole::Phrase2;
    if (has("phrase"))               return uasf::TrackRole::Phrase1;

    // Named instrument (Piano, Strings, Tbn, ...) with no structural keyword.
    // NTR/NTT alone do not determine a musical role, so we do not guess.
    return uasf::TrackRole::Accompaniment;
}

uasf::SectionType Sff1ToUasfMapper::mapCasmSectionType(const std::string& name) noexcept {
    std::string n;
    n.reserve(name.size());
    for (char c : name)
        n.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    auto has = [&](const char* s) { return n.find(s) != std::string::npos; };

    if (has("intro")) {
        if (has(" c")) return uasf::SectionType::Intro3;
        if (has(" b")) return uasf::SectionType::Intro2;
        return uasf::SectionType::Intro1;
    }
    if (has("ending")) {
        if (has(" c")) return uasf::SectionType::Ending3;
        if (has(" b")) return uasf::SectionType::Ending2;
        return uasf::SectionType::Ending1;
    }
    if (has("break")) return uasf::SectionType::Break;
    if (has("fill")) {
        // "Fill In AA/BB/CC/DD" map to the matching main variant.
        if (has("bb")) return uasf::SectionType::Fill2;
        if (has("cc")) return uasf::SectionType::Fill3;
        if (has("dd")) return uasf::SectionType::Fill4;
        return uasf::SectionType::Fill1;
    }
    if (has("main")) {
        if (has(" d")) return uasf::SectionType::Main4;
        if (has(" c")) return uasf::SectionType::Main3;
        if (has(" b")) return uasf::SectionType::Main2;
        return uasf::SectionType::Main1;
    }
    return uasf::SectionType::Main1;
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
