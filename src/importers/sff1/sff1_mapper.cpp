#include "importers/sff1/sff1_mapper.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>

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

    // Resolution (PPQN) must match the source so the playback clock plays the
    // event ticks at the correct tempo. The MThd-derived section carries the
    // real SMF division; fall back to 480 only if no section reports one.
    style.resolution = 480;
    for (const auto& s : parseResult.sections) {
        if (s.resolution > 0) { style.resolution = s.resolution; break; }
    }

    // ── Source-channel → CASM config map (Task A: per-role splitting) ──
    // The SFF1 reader collapses the whole style into a single mixed-channel
    // SMF MTrk (one `Phrase1` track). Each MIDI event still carries its
    // source channel, and each CASM Ctb2 entry maps a source channel to a
    // named role (Rhythm1/Bass/Chord1/…). We rebuild per-role UASF tracks by
    // bucketing events by channel and resolving the role from CASM — NOT from
    // a channel heuristic, because drums are not always on GM channel 9
    // (e.g. POP_ACOUSTIC_2 puts Rhythm1 on channel 8). Splitting keeps the
    // per-channel NoteOn-dedupe from collapsing unrelated tracks and stops
    // drums from being chord-transposed, recovering retriggers dropped when
    // everything shared one track (PR #7 known limitation).
    std::map<uint8_t, const CasmTrackConfig*> chanConfig;
    for (const auto& cfg : parseResult.casm_configs) {
        // Defensive (Task D): a Ctb2 source-channel byte must be a valid MIDI
        // channel (0-15). A corrupt/abnormal config byte can hold anything; it
        // can never bind to a real event channel, so drop it with a warning
        // rather than letting it shadow a valid config or wrap silently.
        if (cfg.source_channel > 15) {
            result.warnings.push_back(
                "CASM track '" + cfg.name + "' has out-of-range source channel " +
                std::to_string(static_cast<int>(cfg.source_channel)) +
                " — ignored (expected 0-15)");
            continue;
        }
        // First config for a channel wins — keeps the role stable across the
        // section variants that repeat the same source channel.
        chanConfig.emplace(cfg.source_channel, &cfg);
    }

    // Track channels we have already warned about so the fallback message is
    // emitted once per channel, not once per section.
    std::set<uint8_t> fallbackWarned;

    for (const auto& sffSection : parseResult.sections) {
        uasf::SectionDefinition section;
        section.type = mapSectionType(sffSection.type);
        section.name = sffSection.name;
        section.bars = sffSection.bars;
        section.resolution = sffSection.resolution;
        section.beats_per_bar = 4;
        section.beat_note = 4;

        // Bucket events by MIDI channel, building one UASF track per channel.
        std::map<uint8_t, uasf::TrackDefinition> byChannel;
        for (const auto& sffTrack : sffSection.tracks) {
            for (const auto& sffEv : sffTrack.events) {
                const uint8_t ch = sffEv.status & 0x0F;
                auto it = byChannel.find(ch);
                if (it == byChannel.end()) {
                    uasf::TrackDefinition track;
                    track.midi_channel = ch;
                    track.articulation.profile = uasf::ArticulationProfile::Generic;
                    track.articulation.fidelity = uasf::FidelityRequirement::High;

                    auto cit = chanConfig.find(ch);
                    if (cit != chanConfig.end()) {
                        const CasmTrackConfig& cfg = *cit->second;
                        track.name = cfg.name;
                        track.role = mapCasmTrackRole(cfg);
                        track.articulation.ntr = cfg.ntr;
                        track.articulation.ntt = cfg.ntt;
                    } else {
                        // No CASM metadata for this channel: fall back to the
                        // GM drum convention, else treat as a melodic phrase.
                        track.name = "Channel " + std::to_string(static_cast<int>(ch));
                        track.role = (ch == 9) ? uasf::TrackRole::Drum
                                               : uasf::TrackRole::Phrase1;
                        if (fallbackWarned.insert(ch).second) {
                            result.warnings.push_back(
                                "Channel " + std::to_string(static_cast<int>(ch)) +
                                " has events but no CASM metadata — using " +
                                (ch == 9 ? "drum" : "melodic") + " fallback role");
                        }
                    }
                    track.is_drum = (track.role == uasf::TrackRole::Drum ||
                                     track.role == uasf::TrackRole::Percussion);
                    if (track.role == uasf::TrackRole::Bass ||
                        track.role == uasf::TrackRole::Percussion) {
                        track.articulation.fidelity =
                            uasf::FidelityRequirement::Medium;
                    }
                    it = byChannel.emplace(ch, std::move(track)).first;
                }
                it->second.events.push_back(mapMidiEvent(sffEv));
            }
        }

        // std::map iterates in ascending channel order → deterministic output.
        for (auto& [ch, track] : byChannel) {
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
