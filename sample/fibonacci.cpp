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
#include <cstdlib>

// -----------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Fibonacci_sequence
int fibonacci(int n)
{
  giopler::dev::Function function(n);
  if (n <= 1)   return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

// -----------------------------------------------------------------------------
// Note: As written, this program will send 362 events to the Giopler servers.
int main()
{
  giopler::dev::confirm(fibonacci( 1) ==    1);
  giopler::dev::confirm(fibonacci(10) ==   55);   // 362 events up to here
  //giopler::dev::confirm(fibonacci(13) ==  233);   // 1869 events up to here
  //giopler::dev::confirm(fibonacci(19) == 4181);   // 28928 events up to here

  return EXIT_SUCCESS;
}
