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

#include <string.h>
#include "ZrHa64-common.h"

static inline uint64_t load64le(const void *p)
{
    uint64_t x;
    memcpy(&x, p, 8);
#if defined(__GNUC__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    x = __builtin_bswap64(x);
#endif
    return x;
}

static inline uint64_t rotl64(uint64_t x, int k)
{
    return x << k | x >> (64 - k);
}

static inline void ZrHa_update(uint64_t state[2], uint64_t data[2])
{
    uint64_t x0 = state[0] + data[0];
    uint64_t x1 = state[1] + data[1];
    uint64_t m0 = (uint32_t) x0 * (x0 >> 32);
    uint64_t m1 = (uint32_t) x1 * (x1 >> 32);
    state[0] = m0 + rotl64(x1, 32);
    state[1] = m1 + rotl64(x0, 32);
}

static inline void ZrHa_feed(uint64_t state[2], const char *p)
{
    uint64_t data[2];
    data[0] = load64le(p + 0);
    data[1] = load64le(p + 8);
    ZrHa_update(state, data);
}

static inline void ZrHa_merge(uint64_t state[2], const uint64_t other[2])
{
    uint64_t x0 = state[0] + rotl64(other[1], 32);
    uint64_t x1 = state[1] + rotl64(other[0], 32);
    uint64_t m0 = (uint32_t) x0 * (x0 >> 32);
    uint64_t m1 = (uint32_t) x1 * (x1 >> 32);
    state[0] = m0 + x1;
    state[1] = m1 + x0;
}

static inline void ZrHa_addlen(uint64_t state[2], uint32_t len)
{
    len *= 2654435761;
    state[0] ^= len;
    state[1] ^= len;
}

static inline uint64_t ZrHa_final(uint64_t state[2])
{
    return state[0] + rotl64(state[1], 32);
}

uint64_t ZrHa64_long_generic(const void *data, size_t len, uint64_t seed0, uint64_t seed1)
{
    uint64_t state[8] = {
	ZrHa64_IV[0] ^ seed0,
	ZrHa64_IV[1] ^ seed1,
	ZrHa64_IV[2] ^ seed0,
	ZrHa64_IV[3] ^ seed1,
	ZrHa64_IV[4] ^ seed0,
	ZrHa64_IV[5] ^ seed1,
	ZrHa64_IV[6] ^ seed0,
	ZrHa64_IV[7] ^ seed1,
    };
    const char *p = data;
    const char *last32 = p + len - 32;
    if (len & 32) {
	ZrHa_feed(state + 4, p + 0);
	ZrHa_feed(state + 6, p + 16);
	p += 32;
    }
    do {
	ZrHa_feed(state + 0, p + 0);
	ZrHa_feed(state + 2, p + 16);
	ZrHa_feed(state + 4, p + 32);
	ZrHa_feed(state + 6, p + 48);
	p += 64;
    } while (p < last32);
    ZrHa_feed(state + 0, last32 + 0);
    ZrHa_feed(state + 2, last32 + 16);
    // down to a single AVX2 register
    ZrHa_merge(state + 0, state + 4);
    ZrHa_merge(state + 2, state + 6);
    ZrHa_addlen(state + 0, len);
    // down to a single SSE2 register
    ZrHa_merge(state + 0, state + 2);
    return ZrHa_final(state);
}
