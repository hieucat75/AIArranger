#include "engine/arranger/style_player.h"
#include <cstdio>
#include <cstring>
#include <cmath>

namespace ai_arranger::arranger {

StylePlayer::StylePlayer(realtime::RealtimeClock& clock,
                          midi::MidiScheduler& scheduler,
                          midi::PanicHandler& panic)
    : clock_(clock)
    , scheduler_(scheduler)
    , panic_handler_(panic) {}

void StylePlayer::loadStyle(const uasf::StyleDefinition& style) noexcept {
    style_ = style;
    style_loaded_ = true;
    sequencer_.setSections(style_.sections.data(), style_.sections.size());
}

void StylePlayer::clearStyle() noexcept {
    style_loaded_ = false;
    style_ = {};
}

bool StylePlayer::start(int introSectionIndex) noexcept {
    if (!style_loaded_ || style_.sections.empty()) return false;

    // Find intro section
    if (introSectionIndex < 0 || introSectionIndex >= static_cast<int>(style_.sections.size())) {
        introSectionIndex = 0;
    }

    clock_.setTempo(style_.tempo_bpm);
    clock_.reset();
    clock_.start();

    sequencer_.setCurrentChord(chords::CMaj);
    current_event_index_ = 0;
    last_dispatched_tick_ = 0;

    // Queue intro section
    sequencer_.queueSection(introSectionIndex);

    if (event_cb_) event_cb_("START");

    return true;
}

void StylePlayer::stop() noexcept {
    clock_.stop();
    sequencer_.panic();
    if (event_cb_) event_cb_("STOP");
}

void StylePlayer::panic() noexcept {
    scheduler_.clear();
    panic_handler_.panic(scheduler_, [this](const uasf::MidiEvent& ev) {
        scheduler_.scheduleEvent(ev);
    });
    sequencer_.panic();
    clock_.stop();
    if (event_cb_) event_cb_("PANIC");
}

void StylePlayer::fill() noexcept {
    sequencer_.queueFill();
    if (event_cb_) event_cb_("FILL_QUEUED");
}

void StylePlayer::ending() noexcept {
    sequencer_.queueEnding();
    if (event_cb_) event_cb_("ENDING_QUEUED");
}

void StylePlayer::switchSection(int index) noexcept {
    sequencer_.queueSection(index);
    if (event_cb_) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "SECTION_QUEUED=%d", index);
        event_cb_(buf);
    }
}

void StylePlayer::setChord(Chord chord) noexcept {
    chord_input_.setChord(chord);
    sequencer_.setCurrentChord(chord);
    if (event_cb_) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CHORD=%d,%d", static_cast<int>(chord.type), chord.root);
        event_cb_(buf);
    }
}

Chord StylePlayer::getCurrentChord() const noexcept {
    return sequencer_.getCurrentChord();
}

void StylePlayer::tick() noexcept {
    if (!style_loaded_ || !clock_.isRunning()) return;

    int64_t currentTick = clock_.getPosition();
    int64_t barSize = clock_.ticksPerBar();

    // Advance section sequencer (check bar boundary)
    bool sectionSwitched = sequencer_.advance(currentTick, barSize);
    if (sectionSwitched) {
        current_event_index_ = 0;
    }

    // Get current section
    int sectionIdx = sequencer_.getCurrentSectionIndex();
    if (sectionIdx < 0 || sectionIdx >= static_cast<int>(style_.sections.size())) return;

    const auto& section = style_.sections[sectionIdx];
    Chord currentChord = sequencer_.getCurrentChord();

    // Dispatch events for this section
    dispatchSectionEvents(section, currentTick, currentChord);
}

bool StylePlayer::isPlaying() const noexcept {
    return clock_.isRunning();
}

int StylePlayer::getCurrentSection() const noexcept {
    return sequencer_.getCurrentSectionIndex();
}

const uasf::SectionDefinition* StylePlayer::getCurrentSectionDef() const noexcept {
    int idx = sequencer_.getCurrentSectionIndex();
    if (idx < 0 || idx >= static_cast<int>(style_.sections.size())) return nullptr;
    return &style_.sections[idx];
}

SequencerState StylePlayer::getState() const noexcept {
    return sequencer_.getState();
}

void StylePlayer::setEventCallback(PlaybackEventCallback cb) noexcept {
    event_cb_ = std::move(cb);
}

void StylePlayer::dispatchSectionEvents(const uasf::SectionDefinition& section,
                                          int64_t currentTick,
                                          Chord chord) noexcept {
    bool dispatchedAny = false;

    for (; current_event_index_ < section.tracks.size(); ++current_event_index_) {
        const auto& track = section.tracks[current_event_index_];
        for (const auto& event : track.events) {
            if (event.tick > currentTick - last_dispatched_tick_) break;

            // Transpose note based on chord and track role
            uasf::MidiEvent dispatched = event;
            if (dispatched.type == uasf::MidiEventType::NoteOn ||
                dispatched.type == uasf::MidiEventType::NoteOff) {
                dispatched.data1 = transposeNote(dispatched.data1, chord, track.role);
            }

            // Adjust tick to be relative to current playback position
            dispatched.tick = event.tick + last_dispatched_tick_;

            scheduler_.scheduleEvent(dispatched);
            dispatchedAny = true;
        }
    }

    if (dispatchedAny) {
        last_dispatched_tick_ = currentTick;
    }
}

uint8_t StylePlayer::transposeNote(uint8_t note, Chord chord, uasf::TrackRole role) const noexcept {
    // For now: simple root-based transposition
    // Drum tracks are not transposed
    if (role == uasf::TrackRole::Drum || role == uasf::TrackRole::Percussion) {
        return note;
    }

    if (chord.type == ChordType::NoChord) return note;

    // Simple: shift by chord root offset from C (60)
    int8_t offset = static_cast<int8_t>(chord.root) - 60;
    int16_t result = static_cast<int16_t>(note) + offset;

    // Clamp to valid MIDI range
    if (result < 0) result = 0;
    if (result > 127) result = 127;

    return static_cast<uint8_t>(result);
}

// ── Demo style builder ─────────────────────────────────────────────

static uasf::MidiEvent makeEvent(uasf::MidiEventType type, uint8_t ch,
                                  uint8_t d1, uint8_t d2, uint64_t tick) {
    uasf::MidiEvent ev;
    ev.type = type;
    ev.channel = ch;
    ev.data1 = d1;
    ev.data2 = d2;
    ev.tick = tick;
    return ev;
}

static uasf::TrackDefinition makeDrumTrack(uint64_t tickOffset) {
    uasf::TrackDefinition track;
    track.name = "Drums";
    track.midi_channel = 9;
    track.role = uasf::TrackRole::Drum;
    track.is_drum = true;

    // Basic rock beat: kick (36) on 1&3, snare (38) on 2&4, hihat (42) every 8th
    for (int bar = 0; bar < 4; ++bar) {
        int64_t barStart = bar * 1920; // 4 beats * 480 ticks
        // Kick: beat 1, 3
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 9, 36, 100, tickOffset + barStart));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 9, 36, 0, tickOffset + barStart + 240));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 9, 36, 100, tickOffset + barStart + 960));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 9, 36, 0, tickOffset + barStart + 1200));
        // Snare: beat 2, 4
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 9, 38, 90, tickOffset + barStart + 480));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 9, 38, 0, tickOffset + barStart + 720));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 9, 38, 90, tickOffset + barStart + 1440));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 9, 38, 0, tickOffset + barStart + 1680));
        // Hi-hat: every 8th note
        for (int eighth = 0; eighth < 8; ++eighth) {
            int64_t hihat = tickOffset + barStart + eighth * 240;
            track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 9, 42, 80, hihat));
            track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 9, 42, 0, hihat + 120));
        }
    }
    return track;
}

static uasf::TrackDefinition makeBassTrack(uint64_t tickOffset) {
    uasf::TrackDefinition track;
    track.name = "Bass";
    track.midi_channel = 1;
    track.role = uasf::TrackRole::Bass;
    track.is_drum = false;

    // Simple bass line: root on beat 1, 5th on beat 3
    for (int bar = 0; bar < 4; ++bar) {
        int64_t barStart = bar * 1920;
        // Root note (C3 = 48) on beat 1
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 1, 48, 90, tickOffset + barStart));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 1, 48, 0, tickOffset + barStart + 480));
        // 5th (G3 = 55) on beat 3
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 1, 55, 80, tickOffset + barStart + 960));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 1, 55, 0, tickOffset + barStart + 1440));
    }
    return track;
}

static uasf::TrackDefinition makeChordTrack(uint64_t tickOffset) {
    uasf::TrackDefinition track;
    track.name = "Chord";
    track.midi_channel = 2;
    track.role = uasf::TrackRole::Chord;
    track.is_drum = false;

    // Simple held chords: C Major (60, 64, 67) held for 2 bars
    for (int bar = 0; bar < 4; ++bar) {
        int64_t barStart = bar * 1920;
        // Chord voicing (transposed by style player based on chord)
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 2, 60, 70, tickOffset + barStart));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 2, 64, 70, tickOffset + barStart));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOn, 2, 67, 70, tickOffset + barStart));
        // Hold for 2 beats
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 2, 60, 0, tickOffset + barStart + 960));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 2, 64, 0, tickOffset + barStart + 960));
        track.events.push_back(makeEvent(uasf::MidiEventType::NoteOff, 2, 67, 0, tickOffset + barStart + 960));
    }
    return track;
}

uasf::StyleDefinition buildDemoStyle() noexcept {
    uasf::StyleDefinition style;
    style.name = "Gate 2 Demo Style (Rock)";
    style.format_version = "1.0";
    style.tempo_bpm = 120;
    style.resolution = 480;

    // Intro section (4 bars)
    uasf::SectionDefinition intro;
    intro.type = uasf::SectionType::Intro1;
    intro.name = "Intro";
    intro.bars = 4;
    intro.resolution = 480;
    intro.beats_per_bar = 4;
    intro.beat_note = 4;
    intro.tracks.push_back(makeDrumTrack(0));
    intro.tracks.push_back(makeBassTrack(0));
    style.sections.push_back(std::move(intro));

    // Main section (4 bars)
    uasf::SectionDefinition main;
    main.type = uasf::SectionType::Main1;
    main.name = "Main";
    main.bars = 4;
    main.resolution = 480;
    main.beats_per_bar = 4;
    main.beat_note = 4;
    main.tracks.push_back(makeDrumTrack(0));
    main.tracks.push_back(makeBassTrack(0));
    main.tracks.push_back(makeChordTrack(0));
    style.sections.push_back(std::move(main));

    // Fill section (1 bar)
    uasf::SectionDefinition fill;
    fill.type = uasf::SectionType::Fill1;
    fill.name = "Fill";
    fill.bars = 1;
    fill.resolution = 480;
    fill.beats_per_bar = 4;
    fill.beat_note = 4;
    fill.tracks.push_back(makeDrumTrack(0));
    style.sections.push_back(std::move(fill));

    // Ending section (2 bars)
    uasf::SectionDefinition ending;
    ending.type = uasf::SectionType::Ending1;
    ending.name = "Ending";
    ending.bars = 2;
    ending.resolution = 480;
    ending.beats_per_bar = 4;
    ending.beat_note = 4;
    ending.tracks.push_back(makeDrumTrack(0));
    ending.tracks.push_back(makeBassTrack(0));
    ending.tracks.push_back(makeChordTrack(0));
    style.sections.push_back(std::move(ending));

    return style;
}

} // namespace ai_arranger::arranger
