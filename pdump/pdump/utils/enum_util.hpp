#pragma once

#include <utility>
#include <type_traits>
#include <string_view>
#include <string>
#include <print>

#define __PRETTY_FUNCTION__ __FUNCSIG__

namespace pdm::enum_utils {
    template <auto... Xs, typename F>
    constexpr void for_values(F&& f)
    {
        (f.template operator() < Xs > (), ...);
    }

    template <auto B, auto E, typename F>
    constexpr void for_range(F&& f)
    {
        using t = std::common_type_t<decltype(B), decltype(E)>;

        [&f] <auto... Xs>(std::integer_sequence<t, Xs...>)
        {
            for_values<(B + Xs)...>(f);
        }
        (std::make_integer_sequence<t, E - B>{});
    }

    template <typename Enum, Enum value>
    const char* print_enum_value() {
        static_assert(std::is_integral_v<std::underlying_type_t<Enum>>,
            "Enum should be based on integral types");

        const char* name = __PRETTY_FUNCTION__;
        return name;
    }

    extern char* g_alloc[0x200];

    template <typename Enum, Enum Min, Enum Max>
    void generate_enum_functions() {
        static_assert(std::is_enum_v<Enum>, "Enum type expected");

        for_range<(std::to_underlying(Min)), std::to_underlying(Max) + 1>([]<auto E>()
        {
            g_alloc[E] = (char*)print_enum_value<Enum, static_cast<Enum>(E)>();
        });
    }

    std::string extract_enum_value(const char* input);

    template<typename Enum>
    std::string enum_name(Enum E) {
        static_assert(std::is_enum_v<Enum>, "Enum type expected");

        generate_enum_functions<Enum, static_cast<Enum>(0), static_cast<Enum>(0x200)>();

        auto _Typ = g_alloc[std::to_underlying(E)];
        auto _Nam = extract_enum_value(_Typ);

        return _Nam;
    }
}