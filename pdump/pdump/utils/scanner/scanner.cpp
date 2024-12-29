
#include <algorithm>

#include "scanner.hpp"

namespace pdm {
    auto find_byte(const std::byte* first, const std::byte* last, std::byte byte) {
        return reinterpret_cast<const std::byte*>(
            std::memchr(first, static_cast<int>(byte), last - first));
    }

    const std::byte* scan_impl_normal_x16(const std::byte* begin, const std::byte* end, signature sig) {
        end -= sig.size() - 1;

        const auto first_elem = sig.front();
        if (first_elem.mask() == std::byte{ 0xFF }) {
            const auto first = first_elem.byte();

            auto ptr = find_byte(begin, end, first);

            while (ptr) [[likely]] {
                if (reinterpret_cast<uintptr_t>(ptr) % 16 == 0) [[unlikely]] {
                    if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                        return ptr;
                    }
                }

                ptr = find_byte(ptr + 1, end, first);
            }

            return nullptr;
        }

        for (auto ptr = begin; ptr < end; ptr += 16) {
            if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                return ptr;
            }
        }

        return nullptr;
    }

    const std::byte* scan_impl_normal_x1(const std::byte* begin, const std::byte* end, signature sig) {
        const auto first_elem = sig.front();
        if (first_elem.mask() == std::byte{ 0xFF }) {
            const auto first = first_elem.byte();

            const auto upper_bound = end - (sig.size() - 1);
            auto ptr = find_byte(begin, upper_bound, first);

            while (ptr) [[likely]] {
                if (std::equal(sig.begin(), sig.end(), ptr)) [[unlikely]] {
                    return ptr;
                }

                ptr = find_byte(ptr + 1, upper_bound, first);
            }

            return nullptr;
        }

        auto iter = std::search(begin, end, sig.begin(), sig.end());
        return iter == end ? nullptr : iter;
    }
}