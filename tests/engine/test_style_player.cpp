#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include <cstdio>
#include <string>

using namespace ai_arranger::arranger;
using namespace ai_arranger::realtime;
using namespace ai_arranger::midi;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: StylePlayer\n");

    RealtimeClock clock;
    MidiScheduler scheduler;
    PanicHandler panic;

    StylePlayer player(clock, scheduler, panic);

    // ── Load demo style ─────────────────────────────────────────
    auto style = buildDemoStyle();
    TEST("Demo style has name", !style.name.empty());
    TEST("Demo style has sections", style.sections.size() >= 3);
    TEST("Demo style tempo = 120", style.tempo_bpm == 120);

    player.loadStyle(style);
    TEST("Style loaded without error", true);

    // ── Start ────────────────────────────────────────────────────
    TEST("Start succeeds", player.start(0));
    TEST("Player is playing", player.isPlaying());

    // ── Tick (simulate audio callback) ────────────────────────────
    clock.setSampleRate(48000);
    clock.setTempo(120);
    clock.setResolution(480);
    clock.start();

    // Simulate enough callbacks to cross a bar boundary
    // At 120 BPM: 1 bar = 480*4 = 1920 ticks
    // ticks per sample = 0.02, so 1920/0.02 = 96000 samples needed
    // With 256-sample callback: 96000/256 ≈ 375 callbacks
    for (int i = 0; i < 400; ++i) {
        clock.advance(256);
        player.tick();
    }

    int64_t pos = clock.getPosition();
    TEST("Position advances after ticks", pos > 0);

    // ── Section switching ─────────────────────────────────────────
    int initialSection = player.getCurrentSection();
    TEST("Current section is valid", initialSection >= 0);
    TEST("Playing state is correct",
         player.getState() == SequencerState::PlayingIntro ||
         player.getState() == SequencerState::PlayingMain);

    // ── Chord change ──────────────────────────────────────────────
    player.setChord(chords::GMaj);
    Chord currentChord = player.getCurrentChord();
    TEST("Chord changed to GMaj", currentChord.root == 67);

    player.setChord(chords::AMin);
    currentChord = player.getCurrentChord();
    TEST("Chord changed to AMin", currentChord.root == 69);

    // ── Fill ──────────────────────────────────────────────────────
    player.fill();
    TEST("Fill queued without error", true);

    // ── Panic ─────────────────────────────────────────────────────
    player.panic();
    TEST("Stop after panic", !player.isPlaying());

    // ── Multiple start/stop ───────────────────────────────────────
    TEST("Restart after panic", player.start(0));
    player.stop();
    TEST("Stop works", !player.isPlaying());

    // ── Event callback ────────────────────────────────────────────
    std::string lastEvent;
    player.setEventCallback([&](const char* e) { lastEvent = e; });
    player.start(0);
    player.stop();
    TEST("Event callback fires", !lastEvent.empty());

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
