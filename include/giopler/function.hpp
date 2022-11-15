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
#ifndef GIOPLER_FUNCTION_HPP
#define GIOPLER_FUNCTION_HPP

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
/// keeps track of nested Function instances
// event_id is the UUID for the exit event of the Function
// this is an internal implementation detail of the Function class
class Trace final
{
 public:
  explicit Trace(std::string_view event_id)
  : _event_id{event_id}
  {
    _stack_depth++;
    _parent_trace_object  = _trace_object;
    _trace_object         = this;
  }

  ~Trace() {
    _trace_object = _parent_trace_object;
    _stack_depth--;
  }

  /// generates the JSON compatible function call stack
  // [0] = program, [1] = thread
  // [<program event id>, <thread event id>, <function event id>, ...]
  std::shared_ptr<giopler::Array> get_stack_array() {
    std::shared_ptr<giopler::Array> stack;
    stack->reserve(_stack_depth);
    std::size_t current_stack_frame = _stack_depth-1;
    Trace* trace_object             = this;

    while (trace_object) {
      (*stack)[current_stack_frame--] = trace_object->_event_id;
      trace_object                    = trace_object->_parent_trace_object;
    }

    return stack;
  }

 private:
  static inline thread_local Trace* _trace_object = nullptr;
  static inline thread_local std::uint32_t _stack_depth   = 0;
  Trace* _parent_trace_object;
  std::string _event_id;
};

// -----------------------------------------------------------------------------
/// keeps track of the profiling performance counters for Function
// this is an internal implementation detail of the Function class
class Profile final
{
 public:
  explicit Profile() {
    Timestamp start_time  = now();
    _event_counters_start = std::make_shared<Record>(read_event_counters());
    _event_counters_start->insert({{"prof.duration"s, to_nanoseconds(start_time) }});

    _parent_profile_object  = _profile_object;
    _profile_object         = this;
  }

  ~Profile() {
    _profile_object = _parent_profile_object;
  }

  std::shared_ptr<Record> get_total_counters_record() {
    stop_counters();
    return event_counters_total;
  }

  std::shared_ptr<Record> get_self_counters_record() {
    stop_counters();
    return event_counters_self;
  }

 private:
  bool _frozen = false;
  Profile* _parent_profile_object;
  static inline thread_local Profile* _profile_object = nullptr;

  std::shared_ptr<Record> _event_counters_start;
  std::shared_ptr<Record> _event_counters_children;

  std::shared_ptr<Record> event_counters_total;
  std::shared_ptr<Record> event_counters_self;

  void stop_counters() {
    if (_frozen)   return;
    _frozen = true;

    Timestamp end_time = now();
    event_counters_total = std::make_shared<Record>(read_event_counters());
    event_counters_total->insert({{"prof.duration"s, to_nanoseconds(end_time) }});
    subtract_number_record(*event_counters_total, *_event_counters_start);

    if (_parent_profile_object) {
      add_number_record(*(_parent_profile_object->_event_counters_children), *event_counters_total);
    }

    (*event_counters_total)["prof.duration"s] = ns_to_sec((*event_counters_total)["prof.duration"s].get_integer());
    event_counters_self = std::make_shared<Record>(*event_counters_total);
    subtract_number_record(*event_counters_self, *_event_counters_children);
  }
};

// -----------------------------------------------------------------------------
/// singleton instance for program-wide data
// collects the initial system information
class Program final
{
 public:
    explicit Program([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode != BuildMode::Off) {
        _data = std::make_unique<ProgramData>();

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);

        std::shared_ptr<Record> entry_record =
            get_event_record(source_location, get_uuid(), "trace"sv, "program_entry"sv);
        entry_record->insert({{"program"s, get_program_record()}});
        sink::g_sink_manager.write_record(entry_record);
      }
    }

    ~Program() {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> exit_record =
            get_event_record(*(_data->_source_location), get_uuid(), "trace"sv, "program_exit"sv);

        sink::g_sink_manager.write_record(exit_record);
      }
    }

 private:
  struct ProgramData {
      std::unique_ptr<giopler::source_location> _source_location;
  };

  // use a data object to help minimize impact when not in Dev or Prof build modes
  [[no_unique_address]]   // means could be a zero-length variable
  std::conditional<g_build_mode != BuildMode::Off,
    std::unique_ptr<ProgramData>, void>::type _data;
};

// -----------------------------------------------------------------------------
static inline Program g_program;

// -----------------------------------------------------------------------------
/// one instance per thread
// collects the initial system information
class Thread final
{
 public:
    explicit Thread([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode != BuildMode::Off) {
        _data = std::make_unique<ThreadData>();

        if constexpr (g_build_mode == BuildMode::Prof) {
          _data->_profile = std::make_unique<Profile>();
        }

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);
        _data->_exit_event_id = get_uuid();

        if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
          _data->_trace = std::make_unique<Trace>(_data->_exit_event_id);
        }

        std::shared_ptr<Record> entry_record =
            get_event_record(source_location, get_uuid(), "trace"sv, "thread_entry"sv);
        sink::g_sink_manager.write_record(entry_record);
      }
    }

    ~Thread() {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> exit_record =
            get_event_record(*(_data->_source_location), get_uuid(), "trace"sv, "thread_exit"sv);

        if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
          exit_record->insert({{"stack"s, _data->_trace->get_stack_array()}});
        }

        if constexpr (g_build_mode == BuildMode::Prof) {
          exit_record->insert({{"prof_linux.total"s, _data->_profile->get_total_counters_record()}});
          exit_record->insert({{"prof_linux.self"s,  _data->_profile->get_self_counters_record()}});
        }

        sink::g_sink_manager.write_record(exit_record);
      }
    }

 private:
  struct ThreadData {
      std::unique_ptr<giopler::source_location> _source_location;
      std::string _exit_event_id;
      std::unique_ptr<Trace> _trace;
      std::unique_ptr<Profile> _profile;
  };

  // use a data object to help minimize impact when not in Dev or Prof build modes
  [[no_unique_address]]   // means could be a zero-length variable
  std::conditional<g_build_mode != BuildMode::Off,
    std::unique_ptr<ThreadData>, void>::type _data;
};

// -----------------------------------------------------------------------------
/// keep track of the lifetime of each thread for tracing and profiling purposes
// depends on the counters declared static inline in linux/counters.hpp
// Note: static initialization order fiasco does not apply when the variables are also inline
static inline thread_local Thread g_thread;

// -----------------------------------------------------------------------------
// in Dev mode we only keep track of locations for tracing
// in Prof mode we also keep track of runtimes
// we generate and store the event id (UUID) for the function profile
// this way we can refer to the function instance while it is still running
// workload is also saved in Dev/trace mode
class Function final
{
 public:
    explicit Function([[maybe_unused]] const double workload = 0,
                      [[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode != BuildMode::Off) {
        _data = std::make_unique<FunctionData>();

        if constexpr (g_build_mode == BuildMode::Prof) {
          _data->_profile = std::make_unique<Profile>();
        }

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);
        _data->_workload        = workload;
        _data->_exit_event_id   = get_uuid();

        if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
          _data->_trace = std::make_unique<Trace>(_data->_exit_event_id);
        }

        std::shared_ptr<Record> entry_record =
            get_event_record(source_location, get_uuid(), "trace"sv, "function_entry"sv, workload);
        sink::g_sink_manager.write_record(entry_record);
      }
    }

    ~Function() {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> exit_record =
            get_event_record(*(_data->_source_location), get_uuid(),
                             "trace"sv, "function_exit"sv, _data->_workload);

        if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
          exit_record->insert({{"stack"s, _data->_trace->get_stack_array()}});
        }

        if constexpr (g_build_mode == BuildMode::Prof) {
          exit_record->insert({{"prof_linux.total"s, _data->_profile->get_total_counters_record()}});
          exit_record->insert({{"prof_linux.self"s,  _data->_profile->get_self_counters_record()}});
        }

        sink::g_sink_manager.write_record(exit_record);
      }
    }

 private:
  struct FunctionData {
      std::unique_ptr<giopler::source_location> _source_location;
      std::string _exit_event_id;
      double _workload{};
      std::unique_ptr<Trace> _trace;
      std::unique_ptr<Profile> _profile;
  };

  // use a data object to help minimize impact when not in Dev or Prof build modes
  [[no_unique_address]]   // means could be a zero-length variable
  std::conditional<g_build_mode != BuildMode::Off,
    std::unique_ptr<FunctionData>, void>::type _data;
};

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_FUNCTION_HPP
