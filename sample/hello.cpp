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

// ----------------------------------------------------
void hello(int hellos)
{
  giopler::dev::Function function(hellos);
  giopler::dev::confirm(hellos);
  std::cout << "Hello, world!" << std::endl;
  giopler::dev::line("printed hello");
  if (--hellos)   hello(hellos);
}

// ----------------------------------------------------
// This program sends 15 events to the Giopler servers.
int main()
{
  giopler::dev::Function function;
  hello(2);
  giopler::prod::branch("done with hellos");

  return EXIT_SUCCESS;
}
