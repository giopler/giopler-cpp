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
#include <type_traits>

#include "giopler/counter.hpp"

// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
// in Dev mode we only keep track of locations for tracing
// in Prof mode we also keep track of runtimes
// we generate and store the event id (UUID) for the function profile
// this way we can refer to the function instance while it is still running
// workload is also saved in Dev/trace mode
class Function final {
 public:
  explicit Function([[maybe_unused]] const double workload = 0,
                    [[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
  {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
      _data = std::make_unique<FunctionData>();

      if constexpr (g_build_mode == BuildMode::Prof) {
        _data->_start_time                = now();
        _data->_event_counters_start      = read_event_counters();
      }

      _data->_stack_depth++;
      _data->_workload                  = workload;
      _data->_source_location           = std::make_unique<giopler::source_location>(source_location);
      _data->_function_event_id         = get_uuid();
      _data->_old_parent_function_name  = g_parent_function_name;
      _data->_old_function_name         = g_function_name;
      g_parent_function_name            = g_function_name;
      g_function_name                   = source_location.function_name();

      _data->_parent_function_object    = _data->_function_object;
      _data->_function_object           = this;

      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_message_record(source_location, get_uuid(),
                                "trace"sv, "function_entry"sv, workload, ""sv));
      sink::g_sink_manager.write_record(record);
    }
  }

  ~Function() {
    if constexpr (g_build_mode == BuildMode::Dev) {
      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_message_record(*(_data->_source_location), get_uuid(),
                                "trace"sv, "function_exit"sv, _data->_workload, ""sv));
      sink::g_sink_manager.write_record(record);
    } else if constexpr (g_build_mode == BuildMode::Prof) {
      _data->_duration_total = timestamp_diff(_data->_start_time, now());
      std::shared_ptr<Record> event_counters_total = std::make_shared<Record>(read_event_counters());
      subtract_number_record(*event_counters_total, _data->_event_counters_start);
      event_counters_total->insert({{"prof.duration", _data->_duration_total}});

      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_profile_record(*(_data->_source_location), "profile_linux",
                                _data->_function_event_id, "function"s, _data->_workload, event_counters_total));
      track_child(*event_counters_total);

      std::shared_ptr<Record> event_counters_self = std::make_shared<Record>(*event_counters_total);
      subtract_number_record(*event_counters_self, _data->_event_counters_children);
      record->insert({{"prof.self", event_counters_self}});

      sink::g_sink_manager.write_record(record);
    }

    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
      g_parent_function_name    = _data->_old_parent_function_name;
      g_function_name           = _data->_old_function_name;
      _data->_function_object   = _data->_parent_function_object;
      _data->_stack_depth--;
    }
  }

 private:
  struct FunctionData {
    double _workload;
    Timestamp _start_time;
    std::string _function_event_id;   // event id for profile

    double _duration_total = 0;
    Record _event_counters_start;
    Record _event_counters_children;

    // use unique pointers to minimize their cost if build mode disables the class
    std::unique_ptr<giopler::source_location> _source_location;
    std::string _old_parent_function_name;
    std::string _old_function_name;

    Function* _parent_function_object;
    static inline thread_local Function* _function_object = nullptr;
    static inline thread_local std::uint32_t _stack_depth = 0;
  };

  // use a data object to help minimize impact when not in Dev or Prof build modes
  [[no_unique_address]]   // means could be a zero-length variable
  std::conditional<g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof,
    std::unique_ptr<FunctionData>, void>::type _data;

  // ---------------------------------------------------------------------------
  /// subtract from the calling function our counters
  void track_child(const Record& record_event_counters_total) {
    if (_data->_parent_function_object) {
      add_number_record(_data->_parent_function_object->_data->_event_counters_children, record_event_counters_total);
    }
  }

  // ---------------------------------------------------------------------------
  /// walk the call stack chain and save the function entry UUIDs
  // uses _stack_depth to preallocate the vector and to know where to store the values
  // stack[0]   = thread
  // stack[max] = current function
  giopler::Array get_stack() {
    giopler::Array stack;
    stack.reserve(_data->_stack_depth+1);   // [0]=thread
    std::size_t current_stack_frame = _data->_stack_depth;
    Function* function_object       = this;

    while (function_object) {
      assert(current_stack_frame);
      stack[current_stack_frame--] = function_object->_data->_function_event_id;
      function_object              = function_object->_data->_parent_function_object;
    }

    assert(current_stack_frame == 0);
    stack[0] = g_thread.get_event_id();
    return stack;
  }
};

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PROFILE_HPP
