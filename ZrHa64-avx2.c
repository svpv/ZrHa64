// Copyright (c) 2019 Alexey Tourbin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <immintrin.h>
#include "ZrHa64-common.h"

static inline void ZrHa_update(__m256i *state, const void *data)
{
    __m256i x = _mm256_add_epi64(*state, _mm256_loadu_si256(data));
    __m256i m = _mm256_mul_epu32(x, _mm256_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1)));
    *state = _mm256_add_epi64(m, _mm256_shuffle_epi32(x, _MM_SHUFFLE(0, 1, 2, 3)));
}

static inline void ZrHa_merge2(__m256i *state, const __m256i *other)
{
    __m256i x = _mm256_add_epi64(*state, _mm256_shuffle_epi32(*other, _MM_SHUFFLE(0, 1, 2, 3)));
    __m256i m = _mm256_mul_epu32(x, _mm256_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1)));
    *state = _mm256_add_epi64(m, _mm256_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2)));
}

static inline void ZrHa_merge(__m128i *state, const __m128i *other)
{
    __m128i x = _mm_add_epi64(*state, _mm_shuffle_epi32(*other, _MM_SHUFFLE(0, 1, 2, 3)));
    __m128i m = _mm_mul_epu32(x, _mm_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1)));
    *state = _mm_add_epi64(m, _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2)));
}

static inline void ZrHa_addlen(__m128i *state, uint32_t len)
{
    __m128i xlen = _mm_cvtsi32_si128(len * 2654435761);
    *state = _mm_xor_si128(*state, _mm_shuffle_epi32(xlen, _MM_SHUFFLE(1, 0, 1, 0)));
}

static inline uint64_t ZrHa_final(__m128i state)
{
    state = _mm_add_epi64(state, _mm_shuffle_epi32(state, _MM_SHUFFLE(0, 1, 2, 3)));
    return _mm_cvtsi128_si64(state);
}

uint64_t ZrHa64_long_avx2(const void *data, size_t len, uint64_t seed0, uint64_t seed1)
{
    __m256i state[2];
    __m256i seed01 = _mm256_set_epi64x(seed1, seed0, seed1, seed0);
    state[0] = _mm256_xor_si256(seed01, _mm256_load_si256((void *)(ZrHa64_IV + 0)));
    state[1] = _mm256_xor_si256(seed01, _mm256_load_si256((void *)(ZrHa64_IV + 4)));
    const char *p = data;
    const char *last32 = p + len - 32;
    if (len & 32) {
	ZrHa_update(state + 1, p + 0);
	p += 32;
    }
    do {
	ZrHa_update(state + 0, p + 0);
	ZrHa_update(state + 1, p + 32);
	p += 64;
    } while (p < last32);
    ZrHa_update(state + 0, last32 + 0);
    // down to a single AVX2 register
    ZrHa_merge2(state + 0, state + 1);
    // exodus from AVX2 to SSE2
    __m128i state0 = _mm256_extracti128_si256(state[0], 0);
    __m128i state1 = _mm256_extracti128_si256(state[0], 1);
    _mm256_zeroupper();
    // down to a single SSE2 register
    ZrHa_merge(&state0, &state1);
    return ZrHa_final(state0);
}
