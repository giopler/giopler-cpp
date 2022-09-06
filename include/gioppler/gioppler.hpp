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
#ifndef GIOPPLER_GIOPPLER_HPP
#define GIOPPLER_GIOPPLER_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

// -----------------------------------------------------------------------------
#include "gioppler/config.hpp"
#include "gioppler/platform.hpp"
#include "gioppler/utility.hpp"
#include "gioppler/record.hpp"
#include "gioppler/sink.hpp"

#include "gioppler/contract.hpp"
#include "gioppler/trace.hpp"
#include "gioppler/log.hpp"

#include "gioppler/histogram.hpp"
#include "gioppler/counter.hpp"
#include "gioppler/exit.hpp"

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_GIOPPLER_HPP
