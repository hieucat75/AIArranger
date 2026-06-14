#include "engine/integration/performer_adapter.h"

namespace ai_arranger::integration {

using control::ControlAction;
using performance::Variation;

PerformerAdapter::PerformerAdapter(realtime::RealtimeClock& clock,
                                   arranger::StylePlayer& player) noexcept
    : clock_(clock), player_(player) {}

bool PerformerAdapter::postEvent(const control::ControlEvent& ev) noexcept {
    return queue_.push(ev);
}

void PerformerAdapter::setVariationSections(int a, int b, int c, int d) noexcept {
    var_section_[0] = a; var_section_[1] = b; var_section_[2] = c; var_section_[3] = d;
}

void PerformerAdapter::startPlayback() noexcept {
    sync_.start();
    sm_.apply(performance::PerformerInput::Start);
    player_.start(0);   // StylePlayer also starts the clock
}

void PerformerAdapter::doPanic() noexcept {
    // Reuse the engine panic (single PanicHandler owner) and reset performer
    // module state so the layer is immediately re-armable.
    player_.panic();
    sync_.reset();
    fill_.reset();
    var_.reset();
    sm_.apply(performance::PerformerInput::Panic);
}

void PerformerAdapter::handleControl(const control::ControlEvent& ev) noexcept {
    switch (ev.action) {
        case ControlAction::SyncArm:
            sync_.arm();
            sm_.apply(performance::PerformerInput::Arm);
            break;
        case ControlAction::Start:
            startPlayback();
            break;
        case ControlAction::Stop:
            sync_.requestStop();
            sm_.apply(performance::PerformerInput::Stop);
            player_.stop();
            sync_.notifyStopped();
            sm_.apply(performance::PerformerInput::Stopped);
            break;
        case ControlAction::Fill:
            // Collapse via the module, then forward to the sequencer (bar-quantized).
            fill_.queueFill(ev.param);
            player_.fill();
            break;
        case ControlAction::VariationA:
        case ControlAction::VariationB:
        case ControlAction::VariationC:
        case ControlAction::VariationD: {
            const int idx = static_cast<int>(ev.action) -
                            static_cast<int>(ControlAction::VariationA);
            var_.queue(static_cast<Variation>(idx));
            player_.switchSection(var_section_[idx]);  // sequencer quantizes
            break;
        }
        case ControlAction::Ending:
            player_.ending();
            break;
        case ControlAction::Panic:
            doPanic();
            break;
        default:
            break;  // None / Intro / Tap: no-op here
    }
}

void PerformerAdapter::tick() noexcept {
    // Drain pending control events (lock-free).
    control::ControlEvent ev;
    while (queue_.pop(ev)) handleControl(ev);

    // Keep performer module bookkeeping coherent with the sequencer's own
    // bar-boundary resolution (clears pending intent; sequencer is the authority).
    fill_.onBarBoundary();
    var_.onBarBoundary();

    player_.tick();
}

void PerformerAdapter::noteOn(uint8_t note) noexcept {
    if (split_.zoneOf(note) != performance::Zone::Lower) return; // upper zone: ignore for chord
    if (held_count_ >= kMaxHeld) return;
    // insert keeping ascending order, dedupe
    int i = held_count_;
    while (i > 0 && held_[i - 1] > note) { held_[i] = held_[i - 1]; --i; }
    if (i < held_count_ && held_[i] == note) return; // already held (shifted back)
    held_[i] = note;
    ++held_count_;
    redetectChord();
}

void PerformerAdapter::noteOff(uint8_t note) noexcept {
    int idx = -1;
    for (int i = 0; i < held_count_; ++i) if (held_[i] == note) { idx = i; break; }
    if (idx < 0) return;
    for (int i = idx; i + 1 < held_count_; ++i) held_[i] = held_[i + 1];
    --held_count_;
    // Sync-start arms on the first chord; fire on note-on transitions only.
    redetectChord();
}

void PerformerAdapter::redetectChord() noexcept {
    if (!scan_ || held_count_ == 0) return;
    arranger::Chord c = scan_->detect(held_, static_cast<size_t>(held_count_));
    last_chord_ = c;
    if (c.type == arranger::ChordType::NoChord) return;

    // Sync start: first chord while armed fires playback.
    if (sync_.isArmed() && sync_.onChordPresent()) {
        sm_.apply(performance::PerformerInput::ChordDetected);
        player_.start(0);
    }
    player_.setChord(c);
}

} // namespace ai_arranger::integration
