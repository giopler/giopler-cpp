// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — Copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

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
