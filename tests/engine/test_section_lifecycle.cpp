// Section lifecycle — arranger-engine-spec-v0.9 §5 (state machine):
//   Main loops ("currentTick >= sectionLengthTick -> wrap to 0");
//   Intro does not loop and hands over to Main;
//   Fill / Break play their length, then return to Main;
//   Ending plays its length, then STOPPED.
//
// Before this test the sequencer only switched on explicit commands: after
// Start, the demo style played its 4-bar Intro and then fell silent forever
// (clock running, section stuck at Intro, zero notes) — through the product
// path (LiveEngineFacade) too. Also pinned: loading a style while stopped adopts
// its resolution (a 1920-PPQN Genos import ran on the engine's 480-PPQN clock,
// i.e. 4x too slow).

#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "session/live_engine_facade.h"
#include "importers/sff1/sff1_reader.h"
#include "importers/sff1/sff1_mapper.h"
#include <string>
#include "../midi/fake_midi_output_provider.h"
#include <cstdio>
#include <map>
#include <set>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::arranger;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

namespace {
struct Rig {
    realtime::RealtimeClock clock;
    midi::MidiScheduler     sch;
    midi::PanicHandler      pan;
    StylePlayer             player{clock, sch, pan};
    std::vector<uasf::MidiEvent> out;
    std::map<int64_t, int> sectionAtBar;   // bar -> section index seen at its start
    int64_t tpBar = 0;

    explicit Rig(const uasf::StyleDefinition& st) {
        sch.setOutputCallback([this](const uasf::MidiEvent& e) { out.push_back(e); });
        player.loadStyle(st);
        tpBar = clock.ticksPerBar();
    }
    // Advance `bars` bars in 1 ms-ish steps (48 samples @48 kHz). Returns on the
    // first tick INSIDE the target bar, so a command issued afterwards commits at
    // the following bar boundary.
    void run(int bars) {
        const int64_t endTick = clock.getPosition() + bars * tpBar;
        while (clock.isRunning() && clock.getPosition() < endTick) {
            clock.advance(48);
            player.tick();
            sch.advanceTo(clock.getPosition());
            const int64_t bar = clock.getPosition() / tpBar;
            if (!sectionAtBar.count(bar)) sectionAtBar[bar] = player.getCurrentSection();
        }
    }
    int notesInBar(int64_t bar) const {
        int n = 0;
        for (const auto& e : out)
            if (e.type == uasf::MidiEventType::NoteOn && e.data2 > 0 &&
                static_cast<int64_t>(e.tick) / tpBar == bar) ++n;
        return n;
    }
    int balance() const {
        std::map<int, int> on;
        for (const auto& e : out) {
            const int key = e.channel * 128 + e.data1;
            if (e.type == uasf::MidiEventType::NoteOn && e.data2 > 0) ++on[key];
            else if (e.type == uasf::MidiEventType::NoteOff ||
                     (e.type == uasf::MidiEventType::NoteOn && e.data2 == 0)) --on[key];
        }
        int open = 0;
        for (const auto& [k, v] : on) open += v > 0 ? v : 0;
        return open;
    }
};
} // namespace

int main() {
    std::printf("Test: section lifecycle (loop / auto-advance / fill return / ending stop)\n");
    const auto demo = buildDemoStyle();   // 0 Intro(4) 1 Main(4) 2 Fill(1) 3 Ending(2)

    // ── 1. Intro -> Main, Main loops: accompaniment never falls silent ──
    {
        Rig r(demo);
        r.player.start(0);
        r.run(16);
        bool allBarsSound = true;
        for (int b = 0; b < 16; ++b) if (r.notesInBar(b) == 0) allBarsSound = false;
        TEST("intro->main: every one of 16 bars produces notes", allBarsSound);
        TEST("intro plays bars 0-3", r.sectionAtBar[0] == 0 && r.sectionAtBar[3] == 0);
        TEST("main takes over at bar 4 without a command", r.sectionAtBar[4] == 1);
        TEST("main is still playing at bar 15 (looped twice)", r.sectionAtBar[15] == 1);
        TEST("still running", r.player.isPlaying());

        // Loop is sample-exact: pass 2 (bars 8-11) == pass 1 (bars 4-7) shifted by 4 bars.
        std::multiset<std::tuple<int64_t, int, int>> p1, p2;
        for (const auto& e : r.out) {
            if (e.type != uasf::MidiEventType::NoteOn || e.data2 == 0) continue;
            const int64_t bar = static_cast<int64_t>(e.tick) / r.tpBar;
            if (bar >= 4 && bar < 8)  p1.insert({static_cast<int64_t>(e.tick) - 4 * r.tpBar, e.channel, e.data1});
            if (bar >= 8 && bar < 12) p2.insert({static_cast<int64_t>(e.tick) - 8 * r.tpBar, e.channel, e.data1});
        }
        TEST("main loop repeats identically (no drift, nothing dropped)", !p1.empty() && p1 == p2);
    }

    // ── 2. Fill plays its length (1 bar) then returns to Main ──────────
    {
        Rig r(demo);
        r.player.start(0);
        r.run(5);                 // now inside bar 5 (Main)
        r.player.fill();          // commits at the next bar boundary (bar 6)
        r.run(4);                 // through bar 9
        TEST("fill active at bar 6", r.sectionAtBar[6] == 2);
        TEST("fill returns to main at bar 7", r.sectionAtBar[7] == 1);
        TEST("main keeps sounding after the fill", r.notesInBar(8) > 0 && r.notesInBar(9) > 0);
    }

    // ── 3. Ending plays its length (2 bars) then stops, no stuck notes ─
    {
        Rig r(demo);
        r.player.start(0);
        r.run(5);
        r.player.ending();        // commits at bar 6, 2 bars long (bars 6-7)
        r.run(6);
        TEST("ending active at bar 6", r.sectionAtBar[6] == 3 && r.sectionAtBar[7] == 3);
        TEST("ending stops the transport", !r.player.isPlaying());
        TEST("no notes after the ending", r.notesInBar(8) == 0 && r.notesInBar(9) == 0);
        TEST("every NoteOn has its NoteOff (no stuck notes)", r.balance() == 0);
    }

    // ── 4. A pending user switch still wins over the automatic hand-over ─
    {
        Rig r(demo);
        r.player.start(0);
        r.run(2);
        r.player.switchSection(3);   // user asks for Ending during the Intro (bar 2)
        r.run(1);
        TEST("queued user switch commits at the next bar boundary (ending at bar 3)",
             r.sectionAtBar[3] == 3);
    }

    // ── 5. Loading a style while stopped adopts its resolution ─────────
    {
        auto hi = demo;
        hi.resolution = 1920;
        for (auto& s : hi.sections) {
            s.resolution = 1920;
            for (auto& t : s.tracks) for (auto& e : t.events) e.tick *= 4;
        }
        Rig r(hi);
        TEST("load while stopped adopts 1920 PPQN", r.clock.ticksPerBeat() == 1920);
        r.player.start(0);
        r.run(8);
        TEST("1920-PPQN copy: main reached at bar 4 and sounding",
             r.sectionAtBar[4] == 1 && r.notesInBar(4) > 0 && r.notesInBar(7) > 0);
    }

    // ── 6. Imported Genos style: full length plays, then loops ─────────
    {
        importers::sff1::Sff1Reader reader;
        const auto pr = reader.parseFile(std::string(CORPUS_DIR) +
            "/POP_ACOUSTIC_2_SC_GENOS.S718---7ba40ed3-527f-49ce-a22c-2414d5de2ec5.sty");
        importers::sff1::Sff1ToUasfMapper mapper;
        const auto mr = mapper.map(pr);
        const bool ok = mr.success && !mr.style.sections.empty();
        TEST("import: POP_ACOUSTIC_2 mapped", ok);
        if (ok) {
            const uint32_t bars = mr.style.sections[0].bars;
            TEST("import: section length from content (24 bars, not the 4-bar placeholder)", bars == 24);
            Rig r(mr.style);
            r.player.start(0);
            r.run(static_cast<int>(bars) + 4);
            TEST("import: bar 23 (last) sounds", r.notesInBar(23) > 0);
            TEST("import: loops — bar 24+k repeats bar k", r.notesInBar(bars + 1) == r.notesInBar(1) &&
                                                        r.notesInBar(bars + 2) == r.notesInBar(2) &&
                                                        r.notesInBar(1) > 0);
        }
    }

    // ── 7. Product path: the facade keeps playing past the intro ───────
    {
        midi::FakeMidiOutputProvider out; out.setDevices({{0, "synth"}});
        session::LiveEngineFacade f(nullptr, &out);
        f.selectMidiOutput(0);
        f.start();
        f.transportStart();
        std::map<int, int> perBar; size_t seen = 0;
        for (int i = 0; i < 12 * 2000; ++i) {       // 12 bars @120 BPM, 1 ms ticks
            f.tick(48);
            for (; seen < out.sent.size(); ++seen)
                if (out.sent[seen].type == uasf::MidiEventType::NoteOn && out.sent[seen].data2 > 0)
                    perBar[i / 2000]++;
        }
        bool all = true;
        for (int b = 0; b < 12; ++b) if (perBar[b] == 0) all = false;
        TEST("facade: demo style sounds in all 12 bars", all);
        TEST("facade: snapshot reports Main after the intro", f.snapshot().section == 1);
        f.stop();
    }

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures == 0 ? 0 : 1;
}
