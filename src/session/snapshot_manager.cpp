#include "session/snapshot_manager.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace ai_arranger::session {

namespace {
// Minimal JSON int field reader: finds "key": <int>. Returns false if missing.
bool readInt(const std::string& j, const std::string& key, long& out) {
    const std::string needle = "\"" + key + "\"";
    size_t p = j.find(needle);
    if (p == std::string::npos) return false;
    p = j.find(':', p + needle.size());
    if (p == std::string::npos) return false;
    ++p;
    while (p < j.size() && (j[p] == ' ' || j[p] == '\t')) ++p;
    size_t start = p;
    if (p < j.size() && (j[p] == '-' || j[p] == '+')) ++p;
    while (p < j.size() && j[p] >= '0' && j[p] <= '9') ++p;
    if (p == start) return false;
    try { out = std::stol(j.substr(start, p - start)); } catch (...) { return false; }
    return true;
}
} // namespace

bool SnapshotManager::isValid(const SessionSnapshot& s) noexcept {
    if (s.version != SessionSnapshot::kVersion) return false;
    if (s.variation > 3) return false;
    if (s.tempo_bpm < 20 || s.tempo_bpm > 400) return false;
    if (s.split_point > 127) return false;
    if (s.chord_scan_mode > 3) return false;
    return true;
}

SessionSnapshot SnapshotManager::capture(EngineSession& session,
                                         uint8_t chordScanModeIndex) noexcept {
    SessionSnapshot s;
    auto& a = session.adapter();
    s.variation    = static_cast<uint8_t>(a.variation().current());
    s.tempo_bpm    = session.clock().getTempo();
    s.split_point  = a.split().splitPoint();
    s.manual_bass  = a.split().manualBass();
    s.sync_armed   = a.sync().isArmed();
    s.chord_scan_mode = chordScanModeIndex;
    return s;
}

bool SnapshotManager::restore(const SessionSnapshot& s, EngineSession& session) noexcept {
    if (!isValid(s)) return false;
    auto& a = session.adapter();
    session.clock().setTempo(s.tempo_bpm);
    a.split().setSplitPoint(s.split_point);
    a.split().setManualBass(s.manual_bass);
    a.variation().setImmediate(static_cast<performance::Variation>(s.variation));
    if (s.sync_armed) a.sync().arm();
    return true;
}

std::string SnapshotManager::serialize(const SessionSnapshot& s) {
    std::ostringstream o;
    o << "{\n"
      << "  \"version\": " << s.version << ",\n"
      << "  \"variation\": " << (int)s.variation << ",\n"
      << "  \"tempo_bpm\": " << s.tempo_bpm << ",\n"
      << "  \"split_point\": " << (int)s.split_point << ",\n"
      << "  \"manual_bass\": " << (s.manual_bass ? 1 : 0) << ",\n"
      << "  \"sync_armed\": " << (s.sync_armed ? 1 : 0) << ",\n"
      << "  \"chord_scan_mode\": " << (int)s.chord_scan_mode << "\n"
      << "}\n";
    return o.str();
}

bool SnapshotManager::deserialize(const std::string& json, SessionSnapshot& out) {
    SessionSnapshot s;
    long v = 0;
    if (!readInt(json, "version", v)) return false;
    s.version = static_cast<uint16_t>(v);
    if (readInt(json, "variation", v)) s.variation = static_cast<uint8_t>(v); else return false;
    if (readInt(json, "tempo_bpm", v)) s.tempo_bpm = static_cast<uint32_t>(v); else return false;
    if (readInt(json, "split_point", v)) s.split_point = static_cast<uint8_t>(v); else return false;
    if (readInt(json, "manual_bass", v)) s.manual_bass = (v != 0); else return false;
    if (readInt(json, "sync_armed", v)) s.sync_armed = (v != 0); else return false;
    if (readInt(json, "chord_scan_mode", v)) s.chord_scan_mode = static_cast<uint8_t>(v); else return false;

    if (!isValid(s)) return false;   // reject bad version / out-of-range
    out = s;
    return true;
}

} // namespace ai_arranger::session
