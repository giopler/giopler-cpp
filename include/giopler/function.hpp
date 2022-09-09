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
#ifndef GIOPPLER_PROFILE_HPP
#define GIOPPLER_PROFILE_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <memory>
#include <string>

// -----------------------------------------------------------------------------
#if defined(GIOPLER_PLATFORM_LINUX)
#include "giopler/linux/counter.hpp"
#endif

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
class Function {
 public:
  explicit Function([[maybe_unused]] const double workload = 0,
                    [[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    : _workload{workload},
      _duration_total{},
      _duration_children{}
  {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
      _start_time               = now();
      _source_location          = std::make_unique<giopler::source_location>(source_location);
      _old_parent_function_name = g_parent_function_name;
      _old_function_name        = g_function_name;
      g_parent_function_name    = g_function_name;
      g_function_name           = source_location.function_name();

      _parent_function_object   = _function_object;
      _function_object          = this;
    }

    if constexpr (g_build_mode == BuildMode::Dev) {
      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_event_record(source_location, "trace"sv, "function_entry"sv));
      sink::g_sink_manager.write_record(record);
    } else if constexpr (g_build_mode == BuildMode::Prof) {
      _event_counters_start    = std::make_unique<Record>(read_event_counters());
      _event_counters_children = std::make_unique<Record>();
    }
  }

  ~Function() {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
      g_parent_function_name    = _old_parent_function_name;
      g_function_name           = _old_function_name;
      _function_object          = _parent_function_object;
    }

    if constexpr (g_build_mode == BuildMode::Dev) {
      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_event_record(*_source_location, "trace"sv, "function_exit"sv));
      sink::g_sink_manager.write_record(record);
    } else if constexpr (g_build_mode == BuildMode::Prof) {
      std::shared_ptr<Record> record_total = std::make_shared<Record>(
          create_event_record(*_source_location, "profile"sv, "function_total"sv));
      record_total->insert({{"prof.workload", _workload}});   // workload is same for total and self
      std::shared_ptr<Record> record_self = std::make_shared<Record>(record_total);
      (*record_self)["evt.event"s] = "function_self"s;   // insert() does not overwrite

      _duration_total  = timestamp_diff(_start_time, now());
      Record event_counters_total{read_event_counters()};
      subtract_number_record(event_counters_total, *_event_counters_start);

      track_child(_duration_total, event_counters_total);
      record_total->merge(event_counters_total);
      record_total->insert({{"prof.duration", _duration_total}});
      sink::g_sink_manager.write_record(record_total);

      subtract_number_record(event_counters_total, *_event_counters_children);
      record_self->merge(*_event_counters_children);
      record_self->insert({{"prof.duration", _duration_total-_duration_children}});
      sink::g_sink_manager.write_record(record_self);
    }
  }

  void track_child(const double duration_total, const Record& record_event_counters_total) {
    if (_parent_function_object) {
      _parent_function_object->_duration_children += duration_total;
      add_number_record(*_parent_function_object->_event_counters_children, record_event_counters_total);
    }
  }

 private:
  const double _workload;
  Timestamp _start_time;
  double _duration_total;
  double _duration_children;

  // use shared pointers to minimize their cost if build mode disables the class
  std::unique_ptr<giopler::source_location> _source_location;
  std::string _old_parent_function_name;
  std::string _old_function_name;

  Function* _parent_function_object;
  static thread_local Function* _function_object;

  std::unique_ptr<Record> _event_counters_start;
  std::unique_ptr<Record> _event_counters_children;
};

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PROFILE_HPP
