#include "session/session_persistence.h"

#include <fstream>
#include <sstream>
#include <string>

namespace ai_arranger::session {

namespace {

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

// Reads "key": "value" (no escape handling needed for our simple values).
bool readStr(const std::string& j, const std::string& key, std::string& out) {
    const std::string needle = "\"" + key + "\"";
    size_t p = j.find(needle);
    if (p == std::string::npos) return false;
    p = j.find(':', p + needle.size());
    if (p == std::string::npos) return false;
    p = j.find('"', p);
    if (p == std::string::npos) return false;
    size_t end = j.find('"', p + 1);
    if (end == std::string::npos) return false;
    out = j.substr(p + 1, end - p - 1);
    return true;
}

std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
    return o;
}

} // namespace

bool SessionPersistence::isValid(const PerformerSession& s) noexcept {
    if (s.version != PerformerSession::kVersion) return false;
    if (s.variation > 3) return false;
    if (s.tempo_bpm < 20 || s.tempo_bpm > 400) return false;
    if (s.split_point > 127) return false;
    if (s.chord_scan_mode > 3) return false;
    if (s.groove_profile > 4) return false;
    if (s.ui_layout > 1) return false;
    return true;
}

std::string SessionPersistence::serialize(const PerformerSession& s) {
    std::ostringstream o;
    o << "{\n"
      << "  \"version\": " << s.version << ",\n"
      << "  \"style_name\": \"" << esc(s.style_name) << "\",\n"
      << "  \"variation\": " << (int)s.variation << ",\n"
      << "  \"tempo_bpm\": " << s.tempo_bpm << ",\n"
      << "  \"split_point\": " << (int)s.split_point << ",\n"
      << "  \"manual_bass\": " << (s.manual_bass ? 1 : 0) << ",\n"
      << "  \"sync_armed\": " << (s.sync_armed ? 1 : 0) << ",\n"
      << "  \"chord_scan_mode\": " << (int)s.chord_scan_mode << ",\n"
      << "  \"groove_profile\": " << (int)s.groove_profile << ",\n"
      << "  \"midi_output_name\": \"" << esc(s.midi_output_name) << "\",\n"
      << "  \"ui_layout\": " << (int)s.ui_layout << ",\n"
      << "  \"theme\": " << (int)s.theme << "\n"
      << "}\n";
    return o.str();
}

bool SessionPersistence::deserialize(const std::string& j, PerformerSession& out) {
    PerformerSession s;
    long v = 0;
    if (!readInt(j, "version", v)) return false; s.version = (uint16_t)v;
    readStr(j, "style_name", s.style_name);   // optional
    if (!readInt(j, "variation", v)) return false; s.variation = (uint8_t)v;
    if (!readInt(j, "tempo_bpm", v)) return false; s.tempo_bpm = (uint32_t)v;
    if (!readInt(j, "split_point", v)) return false; s.split_point = (uint8_t)v;
    if (!readInt(j, "manual_bass", v)) return false; s.manual_bass = (v != 0);
    if (!readInt(j, "sync_armed", v)) return false; s.sync_armed = (v != 0);
    if (!readInt(j, "chord_scan_mode", v)) return false; s.chord_scan_mode = (uint8_t)v;
    if (!readInt(j, "groove_profile", v)) return false; s.groove_profile = (uint8_t)v;
    readStr(j, "midi_output_name", s.midi_output_name); // optional
    if (!readInt(j, "ui_layout", v)) return false; s.ui_layout = (uint8_t)v;
    if (!readInt(j, "theme", v)) return false; s.theme = (uint8_t)v;

    if (!isValid(s)) return false;
    out = s;
    return true;
}

bool SessionPersistence::save(const PerformerSession& s, const std::string& path) {
    if (!isValid(s)) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << serialize(s);
    return f.good();
}

bool SessionPersistence::load(const std::string& path, PerformerSession& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    return deserialize(ss.str(), out);
}

} // namespace ai_arranger::session
