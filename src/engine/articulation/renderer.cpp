#include "engine/articulation/renderer.h"

namespace ai_arranger::articulation {

void NaiveRenderer::render(const uasf::MidiEvent& ev,
                           const uasf::ArticulationMetadata& /*art*/,
                           EventSink& out) const noexcept {
    // Baseline: emit the event unchanged.
    out.emit(ev);
}

void KeyswitchRenderer::render(const uasf::MidiEvent& ev,
                               const uasf::ArticulationMetadata& art,
                               EventSink& out) const noexcept {
    const bool isNoteOn = ev.type == uasf::MidiEventType::NoteOn && ev.data2 > 0;

    // MegaVoice graceful degrade: if the target can't honour keyswitch, pass
    // the note through unchanged (the degradation was logged at construction).
    if (!keyswitch_available_) {
        out.emit(ev);
        return;
    }

    // Only real note-ons on keyswitch tracks get an articulation trigger.
    if (!isNoteOn || !art.has_keyswitch || art.keyswitch_note == ev.data1) {
        out.emit(ev);
        return;
    }

    // Keyswitch note fires strictly before the main note when possible, so a
    // sampler latches the articulation before the note sounds. The trigger is
    // a brief on/off pair on the keyswitch key (same channel).
    const uint64_t ksTick = ev.tick > 0 ? ev.tick - 1 : 0;

    uasf::MidiEvent ksOn = ev;
    ksOn.type    = uasf::MidiEventType::NoteOn;
    ksOn.data1   = art.keyswitch_note;
    ksOn.data2   = kKeyswitchVelocity;
    ksOn.tick    = ksTick;
    out.emit(ksOn);

    uasf::MidiEvent ksOff = ksOn;
    ksOff.type   = uasf::MidiEventType::NoteOff;
    ksOff.data2  = 0;
    ksOff.tick   = ksTick;
    out.emit(ksOff);

    // Then the main note, unchanged.
    out.emit(ev);
}

const NaiveRenderer& defaultRenderer() noexcept {
    static const NaiveRenderer instance;
    return instance;
}

} // namespace ai_arranger::articulation
