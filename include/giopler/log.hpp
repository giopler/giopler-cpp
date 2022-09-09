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
#ifndef GIOPPLER_LOG_HPP
#define GIOPPLER_LOG_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <source_location>
#include <string_view>

#include "giopler/config.hpp"
#include "giopler/utility.hpp"

// -----------------------------------------------------------------------------
namespace giopler::dev
{

// -----------------------------------------------------------------------------
void warning(const std::string_view message,
             const source_location& source_location = source_location::current())
{
  if constexpr (!(g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test)) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "warning"));
    record->insert({{"val.message", message}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void warning(StringFunction auto message_function,
             const source_location& source_location = source_location::current())
{
  if constexpr (!(g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test)) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "warning"));
    record->insert({{"val.message", message_function()}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
namespace giopler::prod
{

// -----------------------------------------------------------------------------
void error(const std::string_view message,
           const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Off) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "error"));
    record->insert({{"val.message", message}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void error(StringFunction auto message_function,
           const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Off) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "error"));
    record->insert({{"val.message", message_function()}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void message(const std::string_view message,
             const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Off) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "message"));
    record->insert({{"val.message", message}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void message(StringFunction auto message_function,
             const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Off) {
    return;
  } else {
    std::shared_ptr<Record> record = std::make_shared<Record>(
        create_event_record(source_location, "log", "message"));
    record->insert({{"val.message", message_function()}});
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::prod

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LOG_HPP
