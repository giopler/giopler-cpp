// Copyright (c) 2023 Giopler
// This code is licensed under the permissive MIT License (MIT).
// SPDX-License-Identifier: MIT-Modern-Variant
// https://fedoraproject.org/wiki/Licensing:MIT#Modern_Variants
//
// Permission is hereby granted, without written agreement and without
// license or royalty fees, to use, copy, modify, and distribute this
// software and its documentation for any purpose, provided that the
// above copyright notice and the following two paragraphs appear in
// all copies of this software.
//
// IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
// DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
// IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
// ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
// PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

#include "giopler/giopler.hpp"
#include <iostream>
#include <cstdlib>
#include <type_traits>

// -----------------------------------------------------------------------------
// is the provided integer an even value?
// https://en.cppreference.com/w/cpp/types/is_integral
template <class T>
constexpr bool is_even(const T i) {
  static_assert(std::is_integral<T>::value, "Integral required.");
  return !(i & 1);
}

// -----------------------------------------------------------------------------
// compute the Collatz conjecture:
//   if number is even, next term is n/2
//   Otherwise the next term is 3*n+1
// https://en.wikipedia.org/wiki/Collatz_conjecture
// returns the number of steps taken to reach one
int collatz(int number)
{
  giopler::dev::Function function;

  if (number == 1)
    return 0;         // done
  else if (is_even(number))
    return 1+collatz(number >> 1);
  else
    return 1+collatz(3*number + 1);
}

// -----------------------------------------------------------------------------
// Note: As written, this program will send 398 events to the Giopler servers.
int main()
{
  giopler::dev::Function function;

  // number of steps to reach one from the index starting value
  const int steps[] = { 0,0, 1, 7, 2, 5, 8, 16, 3, 19,
                        6, 14, 9, 9, 17, 17, 4, 12, 20 };
  constexpr int total_steps = sizeof(steps) / sizeof(int);

  for (int n = 1; n < total_steps; ++n) {
    try {
      giopler::dev::confirm(collatz(n) == steps[n]);
    } catch (const giopler::contract_violation &error) {
      std::cerr << "collatz: index=" << n << " " << error.what() << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
