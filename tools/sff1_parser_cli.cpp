#include "importers/sff1/sff1_reader.h"
#include "importers/sff1/sff1_report.h"
#include "importers/sff1/sff1_mapper.h"
#include "engine/uasf/serializer.h"
#include "engine/uasf/deserializer.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include <iostream>
#include <fstream>
#include <cstdint>

using namespace ai_arranger::importers::sff1;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sff1-parser <file.sty> [--uasf-output <path>] [--json]\n";
        return 1;
    }

    std::string path = argv[1];
    bool jsonOutput = false;
    bool debugCounters = false;
    std::string uasfOutput;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--json") jsonOutput = true;
        if (arg == "--debug-counters") debugCounters = true;
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

    // ── Conversion debug counters (Gate 9 bugfix) ─────────────────────
    // Runs the full pipeline and prints note-event counts at each stage so
    // a loss point is obvious. Read-only; does not change normal behavior.
    if (debugCounters && parseResult.success) {
        using namespace ai_arranger;
        auto countNotes = [](const uasf::StyleDefinition& s, int& sections, int& tracks) {
            sections = static_cast<int>(s.sections.size());
            tracks = 0; int notes = 0;
            for (const auto& sec : s.sections) {
                tracks += static_cast<int>(sec.tracks.size());
                for (const auto& t : sec.tracks)
                    for (const auto& e : t.events)
                        if (e.type == uasf::MidiEventType::NoteOn ||
                            e.type == uasf::MidiEventType::NoteOff) notes++;
            }
            return notes;
        };

        // Source raw note-on/off (from reader sections.tracks)
        int srcNotes = 0, srcOn = 0, srcOff = 0;
        for (const auto& s : parseResult.sections)
            for (const auto& t : s.tracks)
                for (const auto& e : t.events) {
                    uint8_t st = e.status & 0xF0;
                    if (st == 0x90 && e.data2 > 0) { srcOn++; srcNotes++; }
                    else if (st == 0x80 || (st == 0x90 && e.data2 == 0)) { srcOff++; srcNotes++; }
                }

        Sff1ToUasfMapper mapper;
        auto mr = mapper.map(parseResult);
        int mSec = 0, mTrk = 0, mNotes = countNotes(mr.style, mSec, mTrk);

        ai_arranger::uasf::UasfSerializer ser;
        auto sr = ser.serialize(mr.style);

        ai_arranger::uasf::UasfDeserializer de;
        auto dr = de.deserialize(sr.data);
        int dSec = 0, dTrk = 0, dNotes = countNotes(dr.style, dSec, dTrk);

        // Simulated playback dispatch across the whole timeline.
        int dispatched = 0;
        {
            realtime::RealtimeClock clk; midi::MidiScheduler sch; midi::PanicHandler pn;
            arranger::StylePlayer pl(clk, sch, pn);
            sch.setOutputCallback([&](const uasf::MidiEvent& e) {
                if (e.type == uasf::MidiEventType::NoteOn ||
                    e.type == uasf::MidiEventType::NoteOff) dispatched++;
            });
            // Find the max tick so we run long enough to cover all events.
            uint64_t maxTick = 0;
            for (const auto& sec : dr.style.sections)
                for (const auto& t : sec.tracks)
                    for (const auto& e : t.events) if (e.tick > maxTick) maxTick = e.tick;
            pl.loadStyle(dr.style);
            clk.setSampleRate(48000);
            clk.setTempo(dr.style.tempo_bpm ? dr.style.tempo_bpm : 120);
            clk.setResolution(dr.style.resolution ? dr.style.resolution : 480);
            pl.start(0); clk.start();
            int64_t target = static_cast<int64_t>(maxTick) + 8192;
            for (int i = 0; i < 2000000 && clk.getPosition() < target; ++i) {
                clk.advance(2048); pl.tick(); sch.advanceTo(clk.getPosition());
            }
        }

        std::cout << "\n=== conversion debug counters ===\n";
        std::cout << "stage                          | value\n";
        std::cout << "-------------------------------+--------\n";
        std::cout << "reader source MIDI events      | " << parseResult.parsed_events << "\n";
        std::cout << "reader note on/off (raw)       | " << srcNotes
                  << " (on=" << srcOn << " off=" << srcOff << ")\n";
        std::cout << "mapper UASF sections           | " << mSec << "\n";
        std::cout << "mapper UASF tracks             | " << mTrk << "\n";
        std::cout << "mapper UASF note events        | " << mNotes << "\n";
        std::cout << "serializer output (bytes)      | " << sr.data.size() << "\n";
        std::cout << "deserializer note events       | " << dNotes << "\n";
        std::cout << "deserializer resolution        | " << dr.style.resolution << "\n";
        std::cout << "player dispatched note events  | " << dispatched << "\n";
    }

    return parseResult.success ? 0 : 1;
}
