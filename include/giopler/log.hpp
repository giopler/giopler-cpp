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
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Warning, UUID());
    record_log->insert_or_assign("msg"s, message);
    record_log->insert_or_assign("status"s, "Failed");
    sink::g_sink_manager.write_record(record_log);
  }
}

// -----------------------------------------------------------------------------
/// signal a potentially erroneous condition
void warning([[maybe_unused]] StringFunction auto message_function,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test) {
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Warning, UUID());
    record_log->insert_or_assign("msg"s, message_function());
    record_log->insert_or_assign("status"s, "Failed");
    sink::g_sink_manager.write_record(record_log);
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
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Error, UUID());
    record_log->insert_or_assign("msg"s, message);
    record_log->insert_or_assign("status"s, "Failed");
    sink::g_sink_manager.write_record(record_log);
    sink::g_sink_manager.flush();   // user's code could throw and terminate program
  }
}

// -----------------------------------------------------------------------------
/// signal a definitely erroneous condition
void error([[maybe_unused]] StringFunction auto message_function,
           [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Error, UUID());
    record_log->insert_or_assign("msg"s, message_function());
    record_log->insert_or_assign("status"s, "Failed");
    sink::g_sink_manager.write_record(record_log);
    sink::g_sink_manager.flush();   // user's code could throw and terminate program
  }
}

// -----------------------------------------------------------------------------
void message([[maybe_unused]] const std::string_view message = ""sv,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Message, UUID());
    record_log->insert_or_assign("msg"s, message);
    sink::g_sink_manager.write_record(record_log);
  }
}

// -----------------------------------------------------------------------------
void message([[maybe_unused]] StringFunction auto message_function,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record_log =
        get_event_record(source_location, EventCategory::Log, Event::Message, UUID());
    record_log->insert_or_assign("msg"s, message_function());
    sink::g_sink_manager.write_record(record_log);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::prod

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LOG_HPP
