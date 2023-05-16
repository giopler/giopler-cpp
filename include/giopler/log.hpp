// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — Copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

#pragma once
#ifndef GIOPLER_LOG_HPP
#define GIOPLER_LOG_HPP

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
/// signal a potentially erroneous condition
void warning([[maybe_unused]] const std::string_view message = ""sv,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Warning,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message);
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
/// signal a potentially erroneous condition
void warning([[maybe_unused]] StringFunction auto message_function,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Warning,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message_function());
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
namespace giopler::prod
{

// -----------------------------------------------------------------------------
/// signal a definitely erroneous condition
void error([[maybe_unused]] const std::string_view message = ""sv,
           [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Error,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message);
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
/// signal a definitely erroneous condition
void error([[maybe_unused]] StringFunction auto message_function,
           [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Error,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message_function());
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void message([[maybe_unused]] const std::string_view message = ""sv,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Message,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message);
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void message([[maybe_unused]] StringFunction auto message_function,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Log, Event::Message,
                         UUID::get_nil(), UUID::get_nil(), 0, false, message_function());
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::prod

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LOG_HPP
