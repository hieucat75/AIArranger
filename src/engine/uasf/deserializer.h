#ifndef AI_ARRANGER_UASF_DESERIALIZER_H
#define AI_ARRANGER_UASF_DESERIALIZER_H

#include "engine/uasf/types.h"
#include "engine/uasf/format.h"
#include <vector>
#include <cstdint>
#include <string>

namespace ai_arranger::uasf {

struct DeserializeResult {
    bool success;
    StyleDefinition style;
    std::string error;
};

class UasfDeserializer {
public:
    DeserializeResult deserialize(const std::vector<uint8_t>& data) noexcept;
    DeserializeResult deserializeFromFile(const std::string& path) noexcept;

private:
    // ── Binary reader helpers ──────────────────────────────────────
    bool ensure(size_t bytes) noexcept;
    uint8_t  readU8() noexcept;
    uint16_t readU16() noexcept;
    uint32_t readU32() noexcept;
    uint64_t readU64() noexcept;
    void readBytes(void* dest, size_t size) noexcept;
    std::string readString(uint8_t length) noexcept;

    const uint8_t* data_{nullptr};
    size_t size_{0};
    size_t pos_{0};
    bool error_{false};
    std::string error_msg_;
};

} // namespace ai_arranger::uasf
#endif // AI_ARRANGER_UASF_DESERIALIZER_H
