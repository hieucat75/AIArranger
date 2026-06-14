#include "session/engine_session.h"

namespace ai_arranger::session {

EngineSession::EngineSession() noexcept = default;

void EngineSession::loadStyle(const uasf::StyleDefinition& style) noexcept {
    player_.loadStyle(style);
    style_loaded_ = true;
}

bool EngineSession::boot() noexcept {
    if (booted_) return true;  // idempotent

    clock_.setSampleRate(48000);
    clock_.setResolution(480);
    clock_.setTempo(120);
    clock_.reset();

    if (!style_loaded_) {
        loadStyle(arranger::buildDemoStyle());
    }
    booted_ = true;
    return true;
}

void EngineSession::shutdown() noexcept {
    if (!booted_) return;
    player_.stop();    // flushes active notes (no hanging notes)
    player_.panic();   // belt-and-suspenders: clear scheduler + all-notes-off
    clock_.stop();
    booted_ = false;
}

void EngineSession::reset() noexcept {
    shutdown();
    boot();
}

} // namespace ai_arranger::session
