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
#ifndef GIOPLER_PROGRAM_HPP
#define GIOPLER_PROGRAM_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <memory>
#include <string>

// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
class Program
{
 public:
    Program([[maybe_unused]] giopler::source_location source_location = giopler::source_location::current())
    {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_program_record());
        sink::g_sink_manager.write_record(record);
      }

      if constexpr (g_build_mode != BuildMode::Off) {
        _source_location = std::make_unique<giopler::source_location>(source_location);
        _event_id = get_uuid();
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_message_record(source_location, _event_id, "trace"sv, "program_entry"sv, ""sv));
        sink::g_sink_manager.write_record(record);
      }
    }

    ~Program() {
      if constexpr (g_build_mode != BuildMode::Off) {
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_message_record(*_source_location, _event_id, "trace"sv, "program_exit"sv, ""sv));
        sink::g_sink_manager.write_record(record);
      }
    }

 private:
  std::unique_ptr<giopler::source_location> _source_location;
    std::string _event_id;
};

// -----------------------------------------------------------------------------
static inline Program g_program;

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PROGRAM_HPP
