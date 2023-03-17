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
#ifndef GIOPLER_PROFILE_HPP
#define GIOPLER_PROFILE_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>

#include "giopler/counter.hpp"

// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
/// keeps track of nested Function instances
// this is a private class for library internal use only
// event_id is the UUID for the exit event of the Function
// this is an internal implementation detail of the Function class
class Trace final
{
 public:
  explicit Trace(UUID object_id, const char* function_name)
  : _object_id{std::move(object_id)}, _function_name(function_name)
  {
    _stack_depth++;
    _parent_trace_object  = _trace_object;
    _trace_object         = this;
    if (_parent_trace_object)   _parent_trace_object->_is_leaf = false;   // by definition
  }

  ~Trace() {
    _trace_object = _parent_trace_object;
    _stack_depth--;
  }

  /// generates the JSON compatible function id call stack
  // [0]=thread, [_stack_depth-1]=current function
  // [<thread start event id>, <function start event id>, ...]
  std::shared_ptr<giopler::Array> get_object_ids() {
    std::shared_ptr<giopler::Array> stack{std::make_shared<giopler::Array>(_stack_depth)};
    std::size_t current_stack_frame = _stack_depth-1;
    Trace* trace_object             = this;

    while (trace_object) {
      (*stack)[current_stack_frame--] = trace_object->_object_id.get_string();
      trace_object                    = trace_object->_parent_trace_object;
    }

    return stack;
  }

  /// generates the JSON compatible function id call stack
  // [0]=thread, [_stack_depth-1]=current function
  // [<thread start event id>, <function start event id>, ...]
  std::shared_ptr<giopler::Array> get_function_names() {
    std::shared_ptr<giopler::Array> stack{std::make_shared<giopler::Array>(_stack_depth)};
    std::size_t current_stack_frame = _stack_depth-1;
    Trace* trace_object             = this;

    while (trace_object) {
      (*stack)[current_stack_frame--] = trace_object->_function_name;
      trace_object                    = trace_object->_parent_trace_object;
    }

    return stack;
  }

  /// true=this function did not have any children function calls
  [[nodiscard]] bool is_leaf() const {
    return _is_leaf;
  }

 private:
  static inline thread_local Trace* _trace_object       = nullptr;
  static inline thread_local std::uint32_t _stack_depth = 0;
  Trace* _parent_trace_object;
  UUID _object_id;              // object id for this stack frame
  const char* _function_name;   // function name for this stack frame
  bool _is_leaf = true;   // assume true until we know otherwise
};

// -----------------------------------------------------------------------------
/// keeps track of the profiling performance counters for Function
// this is a private class for library internal use only
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
// this is a private class for library internal use only
// collects the initial system information
// tries to match the lifetime of the program
class Program final
{
 public:
    explicit Program([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode != BuildMode::Off) {
        _data = std::make_unique<ProgramData>();
        _data->_source_location = std::make_unique<giopler::source_location>(source_location);

        std::shared_ptr<Record> record_begin =
            get_event_record(source_location, EventCategory::Profile, Event::ProgramBegin,
                             _data->_begin_id);
        record_begin->insert({{"run"s, get_program_record()}});
        sink::g_sink_manager.write_record(record_begin);
      }
    }

    ~Program() {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> record_end =
            get_event_record(*(_data->_source_location), EventCategory::Profile, Event::ProgramEnd,
                             UUID(), _data->_begin_id);

        sink::g_sink_manager.write_record(record_end);
      }
    }

 private:
  struct ProgramData {
      std::unique_ptr<giopler::source_location> _source_location;
      UUID _begin_id;
  };

  // use a data object to help minimize impact when not enabled
  std::unique_ptr<ProgramData> _data;
};

// -----------------------------------------------------------------------------
static inline Program g_program;

// -----------------------------------------------------------------------------
/// one instance per thread
// this is a private class for library internal use only
// tries to match the lifetime of the thread
// we keep trrack of the Trace, but don't bother to report it (empty stack)
class Thread final
{
 public:
    explicit Thread([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
        _data = std::make_unique<ThreadData>();

        if constexpr (g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
          _data->_profile = std::make_unique<Profile>();
        }

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);
        _data->_trace           = std::make_unique<Trace>(_data->_begin_id, "<thread>");

        std::shared_ptr<Record> record_begin =
            get_event_record(source_location, EventCategory::Profile, Event::ThreadBegin,
                             _data->_begin_id);
        record_begin->insert({{"oids"s, _data->_trace->get_object_ids()}});
        record_begin->insert({{"funcs"s, _data->_trace->get_function_names()}});
        sink::g_sink_manager.write_record(record_begin);
      }
    }

    ~Thread() {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
        std::shared_ptr<Record> record_end =
            get_event_record(*(_data->_source_location), EventCategory::Profile, Event::ThreadEnd,
                             UUID(), _data->_begin_id);

        if constexpr (g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
          record_end->insert({{"prof_tot"s, _data->_profile->get_total_counters_record()}});
          record_end->insert({{"prof_self"s,  _data->_profile->get_self_counters_record()}});
        }

        sink::g_sink_manager.write_record(record_end);
      }
    }

    // thread_local variables are at risk of getting optimized away
    static Thread* get_thread() { return &(*g_thread); }

 private:
    // -----------------------------------------------------------------------------
    /// keep track of the lifetime of each thread for tracing and profiling purposes
    // depends on the counters declared static inline in linux/counters.hpp
    // Note: static initialization order fiasco does not apply when the variables are also inline
    // Static variables in one translation unit are initialized according to their definition order.
    static inline thread_local std::unique_ptr<Thread> g_thread{std::make_unique<Thread>()};

    struct ThreadData {
        std::unique_ptr<giopler::source_location> _source_location;
        UUID _begin_id;
        std::unique_ptr<Trace> _trace;
        std::unique_ptr<Profile> _profile;
    };

    // use a data object to help minimize impact when not enabled
    std::unique_ptr<ThreadData> _data;
};

// -----------------------------------------------------------------------------
/// trace or profile a function
// in Dev mode we only keep track of locations for tracing
// in Prof or Bench mode we also keep track of runtimes
// we generate and store the event id (UUID) for the function object
//   this way we can refer to the function instance while it is still running
// workload is a user-supplied amount of work performed estimate
// report stack trace on entry and profile on exit
class Function final
{
 public:
    explicit Function([[maybe_unused]] const double workload = 0,
                      [[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
        _data = std::make_unique<FunctionData>();

        if constexpr (g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
          _data->_profile = std::make_unique<Profile>();
        }

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);
        _data->_workload        = workload;
        _data->_trace           = std::make_unique<Trace>(_data->_begin_id, _data->_source_location->function_name());

        std::shared_ptr<Record> record_begin =
            get_event_record(source_location, EventCategory::Profile, Event::FunctionBegin,
                             _data->_begin_id, UUID::get_nil(), workload);
        record_begin->insert({{"oids"s, _data->_trace->get_object_ids()}});
        record_begin->insert({{"funcs"s, _data->_trace->get_function_names()}});
        sink::g_sink_manager.write_record(record_begin);
      }
    }

    ~Function() {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof || g_build_mode == BuildMode::Bench) {
        std::shared_ptr<Record> record_end =
            get_event_record(*(_data->_source_location), EventCategory::Profile, Event::FunctionEnd,
                             UUID(), _data->_begin_id, _data->_workload, _data->_trace->is_leaf());

        if constexpr (g_build_mode == BuildMode::Prof) {
          record_end->insert({{"prof_tot"s, _data->_profile->get_total_counters_record()}});
          record_end->insert({{"prof_self"s,  _data->_profile->get_self_counters_record()}});
        }

        sink::g_sink_manager.write_record(record_end);
      }
    }

 private:
  static inline Thread* _thread = Thread::get_thread();   // ensure the Thread code is not optimized away

  struct FunctionData {
      std::unique_ptr<giopler::source_location> _source_location;
      UUID _begin_id;
      double _workload{};
      std::unique_ptr<Trace> _trace;
      std::unique_ptr<Profile> _profile;
  };

  // use a data object to help minimize impact when not enabled
  std::unique_ptr<FunctionData> _data;
};

// -----------------------------------------------------------------------------
/// track the lifetime of an object
// report stack trace on entry and profile on exit
// we cannot collect Trace or Profile data
class Object final
{
 public:
    explicit Object([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
        _data = std::make_unique<ObjectData>();

        _data->_source_location = std::make_unique<giopler::source_location>(source_location);

        std::shared_ptr<Record> record_begin =
            get_event_record(source_location, EventCategory::Profile, Event::ObjectBegin,
                             _data->_begin_id);
        sink::g_sink_manager.write_record(record_begin);
      }
    }

    ~Object() {
      if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Prof) {
        std::shared_ptr<Record> record_end =
            get_event_record(*(_data->_source_location), EventCategory::Profile, Event::ObjectEnd,
                             UUID(), _data->_begin_id);

        sink::g_sink_manager.write_record(record_end);
      }
    }

 private:
  struct ObjectData {
      std::unique_ptr<giopler::source_location> _source_location;
      UUID _begin_id;
  };

  // use a data object to help minimize impact when not enabled
  std::unique_ptr<ObjectData> _data;
};

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PROFILE_HPP
