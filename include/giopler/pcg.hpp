// Copyright (c) 2022 Carlos Reyes
// This code is licensed under the permissive MIT License (MIT).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#ifndef GIOPLER_PCG_HPP
#define GIOPLER_PCG_HPP

// -----------------------------------------------------------------------------
namespace giopler {

// -----------------------------------------------------------------------------
// PCG32 random number generator
// Permuted Congruential Generator (PCG)
// minimal implementation that meets the needs of std::random
// http://www.pcg-random.org/
// https://www.pcg-random.org/pdf/hmc-cs-2014-0905.pdf
// https://lemire.me/blog/2017/08/22/testing-non-cryptographic-random-number-generators-my-results/
// https://arvid.io/2018/07/02/better-cxx-prng/
class pcg {
 public:
  // these are here to meet the interface requirements of std::random
  using result_type = uint32_t;
  static constexpr result_type (min)() { return 0; }
  static constexpr result_type (max)() { return UINT32_MAX; }

  explicit pcg() {
    std::random_device rd;   // uses /dev/random on Linux
    const auto s0 = (static_cast<uint64_t>(rd()) << 32) | rd();
    const auto s1 = (static_cast<uint64_t>(rd()) << 32) | rd();

    m_state = 0;
    m_inc = (s0 << 1) | 1;
    (void) operator()();
    m_state += s1;
    (void) operator()();
  }

  result_type operator()() {
    const uint64_t oldstate   = m_state;
    m_state                   = oldstate * 6364136223846793005ULL + m_inc;
    const auto xorshifted     = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
    const auto rot            = static_cast<int>(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }

 private:
  uint64_t m_state;
  uint64_t m_inc;
};

// -----------------------------------------------------------------------------
}   // namespace giopler

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PCG_HPP
