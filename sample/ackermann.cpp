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
// https://en.wikipedia.org/wiki/Ackermann_function
// runs in about 9.5 seconds in debug mode for (4, 1)
uint64_t ackermann(const uint64_t m, const uint64_t n) {
  giopler::dev::Function function(m*n);
  if (m == 0)   return n+1;
  if (n == 0)   return ackermann(m-1, 1);
  return ackermann(m - 1, ackermann(m, n-1));
}

// -----------------------------------------------------------------------------
// Note: As written, this program will send 241 events to the Giopler servers.
int main()
{
  giopler::dev::confirm(ackermann(0, 1) == 2);  // 7 events through here
  giopler::dev::confirm(ackermann(2, 0) == ackermann(1, 1));  // 26 events through here
  giopler::dev::confirm(ackermann(4, 0) == 13);  // 241 events through here
  //giopler::dev::confirm(ackermann(3, 4) == 125);  // 20856 events through here
  //giopler::dev::confirm(ackermann(4, 1) == 65533);

  return EXIT_SUCCESS;
}
