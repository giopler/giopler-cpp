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
namespace gioppler::dev {

// -----------------------------------------------------------------------------
extern Record read_event_counters();

// -----------------------------------------------------------------------------
/// target += other
// assumes all entries are numeric (Integer or Real)
// assumes the other Record contains all key values
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
// assumes the other Record contains all key values
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
  Function([[maybe_unused]] const double workload = 0,
           [[maybe_unused]] gioppler::source_location source_location = gioppler::source_location::current())
  {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
      _source_location          = std::make_unique<gioppler::source_location>(source_location);
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
      Record event_counters_end{read_event_counters()};

      std::shared_ptr<Record> record_total = std::make_shared<Record>(
          create_event_record(*_source_location, "profile"sv, "function_total"sv));
      std::shared_ptr<Record> record_self = std::make_shared<Record>(record_total);
      (*record_self)["evt.event"s] = "function_self"s;

      subtract_number_record(event_counters_end, *_event_counters_start);
      if (_parent_function_object) {
        _parent_function_object->track_child(event_counters_end);
      }
      record_total->merge(event_counters_end);
      sink::g_sink_manager.write_record(record_total);

      subtract_number_record(event_counters_end, *_event_counters_children);
      record_self->merge(event_counters_end);
      sink::g_sink_manager.write_record(record_self);
    }
  }

  void track_child(const Record& child_record) {
    add_number_record(*_event_counters_children, child_record);
  }

 private:
  // use shared pointers to minimize their cost if build mode disables the class
  std::unique_ptr<gioppler::source_location> _source_location;
  std::string _old_parent_function_name;
  std::string _old_function_name;

  Function* _parent_function_object;
  static thread_local Function* _function_object;

  std::unique_ptr<Record> _event_counters_start;
  std::unique_ptr<Record> _event_counters_children;
};

// -----------------------------------------------------------------------------
}   // namespace gioppler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_PROFILE_HPP
