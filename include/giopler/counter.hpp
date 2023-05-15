// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

#pragma once
#ifndef GIOPLER_COUNTER_HPP
#define GIOPLER_COUNTER_HPP

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
#endif // defined GIOPLER_COUNTER_HPP
