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

#include "giopler/giopler.hpp"
#include <cstdlib>

// -----------------------------------------------------------------------------
uint64_t factorial(uint64_t n) {
  giopler::dev::Function function(n);
  return (n <= 1) ? 1 : n * factorial(n-1);
}

// -----------------------------------------------------------------------------
int main()
{
  giopler::dev::confirm(factorial( 0) == 1);
  giopler::dev::confirm(factorial( 1) == 1);
  giopler::dev::confirm(factorial( 3) == 6);
  giopler::dev::confirm(factorial( 5) == 120);
  giopler::dev::confirm(factorial(10) == 3628800);

  return EXIT_SUCCESS;
}
