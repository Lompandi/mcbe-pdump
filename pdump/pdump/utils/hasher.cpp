
#include <format>

#include "hasher.hpp"

namespace pkd {
	__m128i hash(unsigned char* buf, size_t size, uint64_t pseed) {
		uint64_t iv[2]{};

		__m128i hash, seed;

		iv[0] = pseed;
		iv[1] = pseed + size + 1;

		seed = _mm_loadu_si128((__m128i*)iv);

		hash = seed;

		auto len = size;
		while (len) {
			unsigned char tmp[0x50];

			__m128i piece[5]{};

			if (len < 0x50) {
				std::memset(tmp, 0, 0x50);
				std::memcpy(tmp, buf, len);
				buf = tmp;
				len = 0x50;
			}

			piece[0] = _mm_loadu_si128((__m128i*)(buf + 0 * 0x10));
			piece[1] = _mm_loadu_si128((__m128i*)(buf + 1 * 0x10));
			piece[2] = _mm_loadu_si128((__m128i*)(buf + 2 * 0x10));
			piece[3] = _mm_loadu_si128((__m128i*)(buf + 3 * 0x10));
			piece[4] = _mm_loadu_si128((__m128i*)(buf + 4 * 0x10));

			piece[0] = _mm_xor_si128(piece[0], seed);
			piece[1] = _mm_xor_si128(piece[1], seed);
			piece[2] = _mm_xor_si128(piece[2], seed);
			piece[3] = _mm_xor_si128(piece[3], seed);
			piece[4] = _mm_xor_si128(piece[4], seed);

			piece[0] = _mm_aesenc_si128(piece[0], piece[1]);
			piece[0] = _mm_aesenc_si128(piece[0], piece[2]);
			piece[0] = _mm_aesenc_si128(piece[0], piece[3]);
			piece[0] = _mm_aesenc_si128(piece[0], piece[4]);

			piece[0] = _mm_aesenc_si128(piece[0], seed);

			hash = _mm_aesenc_si128(hash, piece[0]);

			buf += 0x50;
			len -= 0x50;
		}

		hash = _mm_aesenc_si128(hash, seed);
		hash = _mm_aesenc_si128(hash, seed);
		hash = _mm_aesenc_si128(hash, seed);
		hash = _mm_aesenc_si128(hash, seed);

		return hash;
	}

	std::string format_m128i(__m128i val) {
		uint64_t sa[2];
		sa[0] = _mm_extract_epi64(val, 0);
		sa[1] = _mm_extract_epi64(val, 1);
		return std::format("{:016x}{:016x}", sa[0], sa[1]);
	}
}