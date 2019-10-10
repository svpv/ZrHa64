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

#include <arm_neon.h>
#include "ZrHa64-common.h"


// TODO: I'm sure this can be optimized more.
// The shuffle is incredibly tricky, and Clang emits excessive vaddhn.i64 instructions
// which I don't like. GCC 9.2.0 actually emits better code here. It's that bad.

static inline uint64x2_t vrevq_u64(uint64x2_t val)
{
    uint32x4_t acc = vrev64q_u32(vreinterpretq_u32_u64(val));
    return vreinterpretq_u64_u32(vextq_u32(acc, acc, 2));
}

static inline void ZrHa_update(uint64x2_t *state, const void *data)
{
    uint64x2_t x = vaddq_u64(*state, vreinterpretq_u64_u8(vld1q_u8(data)));
    uint64x2_t acc = vrevq_u64(x);
    uint32x2_t xlo = vmovn_u64(x);
    uint32x2_t xhi = vshrn_n_u64(x, 32);
    *state = vmlal_u32(acc, xlo, xhi);
}

static inline void ZrHa_merge(uint64x2_t *state, const uint64x2_t *other)
{
    uint64x2_t x = vaddq_u64(*state, vrevq_u64(*other));
    uint64x2_t acc = vrevq_u64(x);
    uint32x2_t xlo = vmovn_u64(x);
    uint32x2_t xhi = vshrn_n_u64(x, 32);
    *state = vmlal_u32(acc, xlo, xhi);
}

static inline void ZrHa_addlen(uint64x2_t *state, uint32_t len)
{
    uint64x2_t xlen = vdupq_n_u64(len * 2654435761);
    *state = veorq_u64(*state, xlen);
}

static inline uint64_t ZrHa_final(uint64x2_t state)
{
#if defined(__aarch64__)
    uint64_t accLo = vgetq_lane_u64(state, 0);
    uint64_t accHi = vgetq_lane_u64(state, 1);
    accLo += (accHi >> 32) | (accHi << 32);
    return accLo;
#else
    uint64x1_t acc = vadd_u64(vget_low_u64(state),
     // eww
     vreinterpret_u64_u32(vrev64_u32(vreinterpret_u32_u64(vget_high_u64(state)))));
    return vget_lane_u64(acc, 0);
#endif
}

uint64_t ZrHa64_long_neon(const void *data, size_t len, uint64_t seed0, uint64_t seed1)
{
    uint64x2_t state[4];
    uint64x2_t seed01 = vcombine_u64(vdup_n_u64(seed0), vdup_n_u64(seed1));
    state[0] = veorq_u64(seed01, vld1q_u64((void *)(ZrHa64_IV + 0)));
    state[1] = veorq_u64(seed01, vld1q_u64((void *)(ZrHa64_IV + 2)));
    state[2] = veorq_u64(seed01, vld1q_u64((void *)(ZrHa64_IV + 4)));
    state[3] = veorq_u64(seed01, vld1q_u64((void *)(ZrHa64_IV + 6)));
    const char *p = data;
    const char *last32 = p + len - 32;
    if (len & 32) {
	ZrHa_update(state + 2, p + 0);
	ZrHa_update(state + 3, p + 16);
	p += 32;
    }
    do {
	ZrHa_update(state + 0, p + 0);
	ZrHa_update(state + 1, p + 16);
	ZrHa_update(state + 2, p + 32);
	ZrHa_update(state + 3, p + 48);
	p += 64;
    } while (p < last32);
    ZrHa_update(state + 0, last32 + 0);
    ZrHa_update(state + 1, last32 + 16);
    // down to a single AVX2 register
    ZrHa_merge(state + 0, state + 2);
    ZrHa_merge(state + 1, state + 3);
    ZrHa_addlen(state + 0, len);
    // down to a single SSE2 register
    ZrHa_merge(state + 0, state + 1);
    return ZrHa_final(state[0]);
}
