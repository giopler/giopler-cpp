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
#ifndef GIOPLER_THREAD_HPP
#define GIOPLER_THREAD_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <memory>
#include <string>
#include "giopler/counter.hpp"

// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
class Thread
{
 public:
    Thread([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode == BuildMode::Dev) {
        _source_location = std::make_unique<giopler::source_location>(source_location);
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_message_record(source_location, "trace"sv, "thread_entry"sv, ""sv));
        sink::g_sink_manager.write_record(record);
      } else if constexpr (g_build_mode == BuildMode::Prof) {
        _start_time           = now();
        _event_counters_start = std::make_unique<Record>(read_event_counters());
        _source_location      = std::make_unique<giopler::source_location>(source_location);
      }
    }

    ~Thread() {
      if constexpr (g_build_mode == BuildMode::Dev) {
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_message_record(*_source_location, "trace"sv, "thread_exit"sv, ""sv));
        sink::g_sink_manager.write_record(record);
      } else if constexpr (g_build_mode == BuildMode::Prof) {
        std::shared_ptr<Record> record_total = std::make_shared<Record>(
            create_profile_record(*_source_location, 0));
        record_total->insert({ "evt.event"s, "thread"sv});

        const double _duration_total = timestamp_diff(_start_time, now());
        Record event_counters_total{read_event_counters()};
        subtract_number_record(event_counters_total, *_event_counters_start);

        record_total->merge(event_counters_total);
        record_total->insert({{"prof.duration", _duration_total}});
        sink::g_sink_manager.write_record(record_total);
      }
    }

 private:
  Timestamp _start_time;
  std::unique_ptr<Record> _event_counters_start;
  std::unique_ptr<giopler::source_location> _source_location;
};

// -----------------------------------------------------------------------------
/// keep track of the lifetime of each thread for logging purposes
// depends on the counters declared static inline in linux/counters.hpp
// static initialization order fiasco does not apply when the variables are also inline
static inline thread_local Thread g_thread;

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_THREAD_HPP
