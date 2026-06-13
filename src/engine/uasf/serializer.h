#ifndef AI_ARRANGER_UASF_SERIALIZER_H
#define AI_ARRANGER_UASF_SERIALIZER_H

#include "engine/uasf/types.h"
#include "engine/uasf/format.h"
#include <vector>
#include <cstdint>

namespace ai_arranger::uasf {

struct SerializeResult {
    bool success;
    std::vector<uint8_t> data;
    std::string error;
};

class UasfSerializer {
public:
    SerializeResult serialize(const StyleDefinition& style) noexcept;

private:
    // ── Binary buffer helpers ──────────────────────────────────────
    void writeU8(uint8_t v) noexcept;
    void writeU16(uint16_t v) noexcept;
    void writeU32(uint32_t v) noexcept;
    void writeU64(uint64_t v) noexcept;
    void writeBytes(const void* data, size_t size) noexcept;
    void writeString(const std::string& s) noexcept;

    // ── Delta encoding ─────────────────────────────────────────────
    uint32_t deltaEncode(uint64_t tick, uint64_t& last_tick) noexcept;

    std::vector<uint8_t> buffer_;
};

} // namespace ai_arranger::uasf
#endif // AI_ARRANGER_UASF_SERIALIZER_H
