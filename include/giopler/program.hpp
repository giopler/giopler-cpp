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
#ifndef GIOPPLER_PROGRAM_HPP
#define GIOPPLER_PROGRAM_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <memory>
#include <string>

// -----------------------------------------------------------------------------
namespace gioppler::dev {

// -----------------------------------------------------------------------------
class Program
{
 public:
    Program([[maybe_unused]] gioppler::source_location source_location = gioppler::source_location::current())
    {
      if constexpr (g_build_mode == BuildMode::Dev) {
        _source_location = std::make_unique<gioppler::source_location>(source_location);
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_event_record(source_location, "trace"sv, "program_entry"sv));
        sink::g_sink_manager.write_record(record);
      }
    }

    ~Program() {
      if constexpr (g_build_mode == BuildMode::Dev) {
        std::shared_ptr<Record> record = std::make_shared<Record>(
            create_event_record(*_source_location, "trace"sv, "program_exit"sv));
        sink::g_sink_manager.write_record(record);
      }
    }

 private:
  std::unique_ptr<gioppler::source_location> _source_location;
};

// -----------------------------------------------------------------------------
static inline Program g_program;

// -----------------------------------------------------------------------------
}   // namespace gioppler::dev

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_PROGRAM_HPP
