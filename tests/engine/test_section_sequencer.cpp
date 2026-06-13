#include "engine/arranger/section_sequencer.h"
#include "engine/uasf/types.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger::arranger;
using namespace ai_arranger::uasf;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: SectionSequencer\n");

    // Build test sections
    std::vector<SectionDefinition> sections(4);
    sections[0].type = SectionType::Intro1; sections[0].name = "Intro"; sections[0].bars = 4;
    sections[1].type = SectionType::Main1;  sections[1].name = "Main";  sections[1].bars = 4;
    sections[2].type = SectionType::Fill1;  sections[2].name = "Fill";  sections[2].bars = 1;
    sections[3].type = SectionType::Ending1;sections[3].name = "Ending";sections[3].bars = 2;
    for (auto& s : sections) { s.resolution = 480; s.beats_per_bar = 4; s.beat_note = 4; }

    SectionSequencer seq;
    seq.setSections(sections.data(), sections.size());

    TEST("Initial state = Stopped", seq.getState() == SequencerState::Stopped);
    TEST("No section selected initially", seq.getCurrentSectionIndex() < 0);

    // Queue section and advance past bar boundary
    seq.queueSection(0);
    TEST("Section queued", seq.isQueued());

    // Advance past first bar boundary (tick 1920 = 4 beats * 480)
    bool switched = seq.advance(2000, 1920);
    TEST("Section switches on bar boundary", switched);
    TEST("Current section = Intro", seq.getCurrentSectionIndex() == 0);

    // Queue Main section
    seq.queueSection(1);
    switched = seq.advance(4000, 1920);
    TEST("Switches to Main", switched && seq.getCurrentSectionIndex() == 1);

    // Queue Fill
    seq.queueFill();
    switched = seq.advance(6000, 1920);
    TEST("Fill queued and switched", switched);
    int fillIdx = seq.getCurrentSectionIndex();
    const auto& fill = sections[fillIdx];
    TEST("Fill section type correct", fill.type >= SectionType::Fill1 && fill.type <= SectionType::Fill4);

    // Queue Ending
    seq.queueEnding();
    switched = seq.advance(8000, 1920);
    TEST("Ending queued and switched", switched);
    int endIdx = seq.getCurrentSectionIndex();
    const auto& end = sections[endIdx];
    TEST("Ending section type correct", end.type >= SectionType::Ending1 && end.type <= SectionType::Ending3);

    // Panic
    seq.panic();
    TEST("Panic clears state", seq.getCurrentSectionIndex() < 0);
    TEST("Panic clears queue", !seq.isQueued());

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
