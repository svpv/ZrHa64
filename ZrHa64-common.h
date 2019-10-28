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

#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif

// Default initialization vector (further to be XORed with seed0/seed1).
extern uint64_t ZrHa64_IV[12];

uint64_t ZrHa64_long_generic(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
uint64_t ZrHa64_long_sse2(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
uint64_t ZrHa64_long_avx2(const void *data, size_t len, uint64_t seed0, uint64_t seed1);

#ifdef __GNUC__
#pragma GCC visibility pop
#endif
