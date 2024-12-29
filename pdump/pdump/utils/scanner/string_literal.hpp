#pragma once

#include <algorithm>
#include <string_view>

namespace pdm {
    template <size_t N>
    struct string_literal {
        constexpr string_literal(const char(&str)[N]) { 
            std::copy_n(str, N, data);
        }

        [[nodiscard]] constexpr std::string_view stringview() const {
            return { data, N - 1 };
        }

        char data[N]{};
    };
}

