#ifndef AI_ARRANGER_ARRANGER_STYLE_PLAYER_H
#define AI_ARRANGER_ARRANGER_STYLE_PLAYER_H

#include "engine/uasf/types.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/arranger/chord_input.h"
#include "engine/arranger/section_sequencer.h"
#include <vector>
#include <functional>

namespace ai_arranger::arranger {

/**
 * Minimal style playback engine for Gate 2.
 *
 * Loads a UASF style definition and plays it through:
 * - RealtimeClock for timing
 * - SectionSequencer for section switching
 * - MidiScheduler for event dispatch
 * - PanicHandler for all-notes-off
 *
 * Architecture rules:
 * - No Yamaha/Korg logic
 * - No internal sound engine
 * - No UI dependency
 * - All realtime paths are lock-free
 */

using PlaybackEventCallback = std::function<void(const char* event)>;

class StylePlayer {
public:
    StylePlayer(realtime::RealtimeClock& clock,
                midi::MidiScheduler& scheduler,
                midi::PanicHandler& panic);

    // ── Style loading (non-realtime) ───────────────────────────────
    void loadStyle(const uasf::StyleDefinition& style) noexcept;
    void clearStyle() noexcept;

    // ── Control ────────────────────────────────────────────────────
    bool start(int introSectionIndex = 0) noexcept;
    void stop() noexcept;
    void panic() noexcept;
    void fill() noexcept;
    void ending() noexcept;
    void switchSection(int index) noexcept;

    // ── Chord ──────────────────────────────────────────────────────
    void setChord(Chord chord) noexcept;
    Chord getCurrentChord() const noexcept;

    // ── Realtime tick — call from audio callback ─────────────────
    void tick() noexcept;

    // ── Status ─────────────────────────────────────────────────────
    bool isPlaying() const noexcept;
    int  getCurrentSection() const noexcept;
    const uasf::SectionDefinition* getCurrentSectionDef() const noexcept;
    SequencerState getState() const noexcept;

    // ── Event callback for demo/debug ──────────────────────────────
    void setEventCallback(PlaybackEventCallback cb) noexcept;

private:
    realtime::RealtimeClock& clock_;
    midi::MidiScheduler&     scheduler_;
    midi::PanicHandler&      panic_handler_;

    uasf::StyleDefinition    style_;
    bool                     style_loaded_{false};
    ChordInput               chord_input_;
    SectionSequencer         sequencer_;

    // Absolute tick at which the current section began playing.
    int64_t section_origin_tick_{0};

    // Last relative tick (within the current section) already dispatched.
    // -1 means "nothing dispatched yet" so tick-0 events still fire.
    int64_t section_rel_cursor_{-1};

    // Event callback for demo/debug
    PlaybackEventCallback event_cb_{nullptr};

    // Dispatch events from the current section that are <= currentTick
    void dispatchSectionEvents(const uasf::SectionDefinition& section,
                               int64_t currentTick,
                               Chord chord) noexcept;

    // Transpose a MIDI event based on chord
    uint8_t transposeNote(uint8_t note, Chord chord, uasf::TrackRole role) const noexcept;
};

// ── Demo style builder ─────────────────────────────────────────────
// Builds a minimal hardcoded style for Gate 2 testing
uasf::StyleDefinition buildDemoStyle() noexcept;

} // namespace ai_arranger::arranger
#endif // AI_ARRANGER_ARRANGER_STYLE_PLAYER_H
