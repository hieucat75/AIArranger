#include "korg_validation/stuck_note_detector.h"

#include <map>

namespace ai_arranger::korg_validation {

StuckResult detectStuck(const MidiCapture& cap) {
    StuckResult r;
    // (channel<<8 | note) -> active flag.
    std::map<int, bool> active;

    for (const auto& e : cap.events) {
        const int key = (static_cast<int>(e.channel()) << 8) | e.data1;
        if (e.isNoteOn()) {
            if (active[key]) ++r.double_on;
            active[key] = true;
        } else if (e.isNoteOff()) {
            if (active[key]) active[key] = false;
            else ++r.orphan_off;
        }
    }

    for (const auto& kv : active) {
        if (kv.second) {
            ++r.hanging;
            r.hanging_notes.emplace_back(kv.first >> 8, kv.first & 0xFF);
        }
    }
    r.balanced = (r.hanging == 0);
    return r;
}

} // namespace ai_arranger::korg_validation
