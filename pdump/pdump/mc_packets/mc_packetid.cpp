

#include "mc_packetid.hpp"
#include <format>

namespace pkd {
    std::optional<uint32_t> get_unsigned_varint(const char* buffer, size_t size) {
        uint32_t varint = 0;
        size_t byte_count = 0;

        for (size_t i = 0; i < size; ++i) {
            uint8_t byte = static_cast<uint8_t>(buffer[i]);
            varint |= (byte & 0x7F) << (7 * byte_count);

            byte_count++;

            if ((byte & 0x80) == 0) {
                return varint;
            }

            if (byte_count > 4) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
}