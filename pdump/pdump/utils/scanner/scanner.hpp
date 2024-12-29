#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

#include "signature.hpp"

//credit to brampedgex @ https://github.com/brampedgex/mnemosyne/

namespace pdm {
	const std::byte* scan_impl_normal_x16(const std::byte* begin, const std::byte* end, signature sig);
	const std::byte* scan_impl_normal_x1(const std::byte* begin, const std::byte* end, signature sig);
}