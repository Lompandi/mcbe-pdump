#pragma once

#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <string>

namespace pkd {
	[[nodiscard]] __m128i hash(unsigned char* buf, size_t size, uint64_t pseed);

	std::string format_m128i(__m128i val);
}