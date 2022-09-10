// Copyright (c) 2022. Carlos Reyes
// This code is licensed under the permissive MIT License (MIT).
// SPDX-License-Identifier: MIT
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
//

#pragma once
#ifndef GIOPPLER_COUNTER_HPP
#define GIOPPLER_COUNTER_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <string>
#include <string_view>

// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
/// target += other
// assumes all entries are numeric (Integer or Real)
// assumes the 'other' Record contains all key values
void add_number_record(Record& target, const Record& other) {
  for (const auto& [key, value] : other) {
    if (value.get_type() == RecordValue::Type::Integer) {
      if (!target.contains(key)) {
        target.insert({key, 0});
      }
      target.at(key) = target.at(key).get_integer() + value.get_integer();
    } else if (value.get_type() == RecordValue::Type::Real) {
      if (!target.contains(key)) {
        target.insert({key, 0.0});
      }
      target.at(key) = target.at(key).get_real() + value.get_real();
    } else {
      assert(false);
    }
  }
}

// -----------------------------------------------------------------------------
/// target -= other
// assumes all entries are numeric (Integer or Real)
// assumes the 'other' Record contains all key values
// clamps values at zero - does not go negative
void subtract_number_record(Record& target, const Record& other) {
  for (const auto& [key, value] : other) {
    if (value.get_type() == RecordValue::Type::Integer) {
      if (!target.contains(key)) {
        target.insert({key, 0});
      }
      if (target.at(key).get_integer() <= value.get_integer()) {
        target.at(key) = 0;
      } else {
        target.at(key) = target.at(key).get_integer() - value.get_integer();
      }
    } else if (value.get_type() == RecordValue::Type::Real) {
      if (!target.contains(key)) {
        target.insert({key, 0.0});
      }
      if (target.at(key).get_real() <= value.get_real()) {
        target.at(key) = 0.0;
      } else {
        target.at(key) = target.at(key).get_real() - value.get_real();
      }
    } else {
      assert(false);
    }
  }
}

// -----------------------------------------------------------------------------
/// read platform-specific performance event counters
// assumed to return the same set of keys on every invocation at a given platform
extern Record read_event_counters();

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#if defined(GIOPLER_PLATFORM_LINUX)
#include "giopler/linux/counter.hpp"
#endif

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_COUNTER_HPP
