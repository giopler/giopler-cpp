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
#ifndef GIOPLER_CONTRACT_HPP
#define GIOPLER_CONTRACT_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <exception>
#include <functional>
#include <memory>
#include <iostream>
#include <source_location>
#include <stdexcept>

#include "giopler/utility.hpp"
#include "giopler/sink.hpp"

// -----------------------------------------------------------------------------
/// Contracts to ensure correct program behavior.
// These print messages to the log and throw exceptions, as needed.
namespace giopler
{

// -----------------------------------------------------------------------------
/// a contract condition has been violated
//
// logic_error are errors that are a consequence of faulty logic
// within the program such as violating logical preconditions or
// class invariants and may be preventable.
class contract_violation : public std::logic_error
{
public:
    explicit contract_violation(const std::string& what_arg)
    : std::logic_error(what_arg) { }

    ~contract_violation() noexcept override = default;

    contract_violation(const contract_violation&) = default;
    contract_violation(contract_violation&&) = default;
    contract_violation& operator=(const contract_violation&) = default;
    contract_violation& operator=(contract_violation&&) = default;
};

// -----------------------------------------------------------------------------
}   // namespace giopler

// -----------------------------------------------------------------------------
/// Contracts to ensure correct program behavior.
// These print messages to the log and throw exceptions, as needed.
namespace giopler::dev
{

// -----------------------------------------------------------------------------
/// errors that arise because an argument value has not been accepted
// the function's expectation of its arguments upon entry into the function
// logs the error and throws exception
void argument([[maybe_unused]] const bool condition,
              [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
    if (!condition) [[unlikely]] {
      std::shared_ptr<Record> record_failed =
          get_event_record(source_location, EventCategory::Contract, Event::Argument, UUID());
      record_failed->insert_or_assign("status"s, "Failed");
      sink::g_sink_manager.write_record(record_failed);
      sink::g_sink_manager.flush();   // throw could terminate program

      const std::string message =
          gformat("ERROR: {}: invalid argument",
                  format_source_location(source_location));
      throw contract_violation{message};
    } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
      std::shared_ptr<Record> record_passed =
          get_event_record(source_location, EventCategory::Contract, Event::Argument, UUID());
      record_passed->insert_or_assign("status"s, "Passed");
      sink::g_sink_manager.write_record(record_passed);
    }
  }
}

// -----------------------------------------------------------------------------
/// expect conditions are like preconditions
// the function's expectation of the state of other objects upon entry into the function
// logs the error and throws exception
void expect([[maybe_unused]] const bool condition,
            [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
    if (!condition) [[unlikely]] {
      std::shared_ptr<Record> record_failed =
          get_event_record(source_location, EventCategory::Contract, Event::Expect, UUID());
      record_failed->insert_or_assign("status"s, "Failed");
      sink::g_sink_manager.write_record(record_failed);
      sink::g_sink_manager.flush();   // throw could terminate program

      const std::string message =
          gformat("ERROR: {}: expect condition failed",
                  format_source_location(source_location));
      throw contract_violation{message};
    } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
      std::shared_ptr<Record> record_passed =
          get_event_record(source_location, EventCategory::Contract, Event::Expect, UUID());
      record_passed->insert_or_assign("status"s, "Passed");
      sink::g_sink_manager.write_record(record_passed);
    }
  }
}

// -----------------------------------------------------------------------------
/// confirms a condition that should be satisfied where it appears in a function body
// logs the error and throws exception
void confirm([[maybe_unused]] const bool condition,
             [[maybe_unused]] const source_location& source_location =
               source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
    if (!condition) [[unlikely]] {
      std::shared_ptr<Record> record_failed =
          get_event_record(source_location, EventCategory::Contract, Event::Confirm, UUID());
      record_failed->insert_or_assign("status"s, "Failed");
      sink::g_sink_manager.write_record(record_failed);
      sink::g_sink_manager.flush();   // throw could terminate program

      const std::string message =
        gformat("ERROR: {}: confirm failed",
               format_source_location(source_location));
      throw contract_violation{message};
    } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
      std::shared_ptr<Record> record_passed =
          get_event_record(source_location, EventCategory::Contract, Event::Confirm, UUID());
      record_passed->insert_or_assign("status"s, "Passed");
      sink::g_sink_manager.write_record(record_passed);
    }
  }
}

// -----------------------------------------------------------------------------
// turn off pesky warning about throwing from inside a catch clause
#if defined(GIOPLER_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate"
#elif defined(GIOPLER_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexceptions"
#endif

// -----------------------------------------------------------------------------
/// invariant condition to check on scope entry and exit
class Invariant final {
 public:
  Invariant() = delete;

  // check invariant on scope entry
  explicit Invariant([[maybe_unused]] BoolFunction auto condition_function,
            [[maybe_unused]] const source_location& source_location =
              source_location::current())
  : _uncaught_exceptions(std::uncaught_exceptions()),
    _condition_function(std::move(condition_function)),
    _source_location(source_location)
  {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
      if (!_condition_function()) [[unlikely]] {
        std::shared_ptr<Record> record_failed =
            get_event_record(source_location, EventCategory::Contract, Event::InvariantBegin, UUID());
        record_failed->insert_or_assign("status"s, "Failed");
        sink::g_sink_manager.write_record(record_failed);
        sink::g_sink_manager.flush();   // throw could terminate program

        const std::string message =
            gformat("ERROR: {}: invariant failed on entry",
                    format_source_location(_source_location));
        throw contract_violation{message};
      } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
        std::shared_ptr<Record> record_passed =
            get_event_record(source_location, EventCategory::Contract, Event::InvariantBegin, UUID());
        record_passed->insert_or_assign("status"s, "Passed");
        sink::g_sink_manager.write_record(record_passed);
      }
    }
  }

  // recheck invariant on scope exit
  // https://en.cppreference.com/w/cpp/error/uncaught_exception
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4152.pdf
  ~Invariant() {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
      if (!_condition_function()) [[unlikely]] {
        try {
          std::shared_ptr<Record> record_failed =
              get_event_record(_source_location, EventCategory::Contract, Event::InvariantEnd, UUID());
          record_failed->insert_or_assign("status"s, "Failed");
          sink::g_sink_manager.write_record(record_failed);
          sink::g_sink_manager.flush();   // throw could terminate program

          const std::string message =
              gformat("ERROR: {}: invariant failed on exit",
                      format_source_location(_source_location));
          throw contract_violation{message};
        } catch (...) {
          if (std::uncaught_exceptions() == _uncaught_exceptions) {
            throw;   // safe to rethrow
          } else {
            // unsafe to throw an exception - discard
          }
        }
      } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
        std::shared_ptr<Record> record_passed =
            get_event_record(_source_location, EventCategory::Contract, Event::InvariantEnd, UUID());
        record_passed->insert_or_assign("status"s, "Passed");
        sink::g_sink_manager.write_record(record_passed);
      }
    }
  }

 private:
  int _uncaught_exceptions;
  std::function<bool()> _condition_function;
  source_location _source_location;
};

// -----------------------------------------------------------------------------
/// ensure postcondition to check on scope exit
class Ensure final {
 public:
  Ensure() = delete;

  explicit Ensure([[maybe_unused]] BoolFunction auto condition_function,
         [[maybe_unused]] const source_location& source_location =
          source_location::current())
  : _uncaught_exceptions(std::uncaught_exceptions()),
    _condition_function(std::move(condition_function)),
    _source_location{source_location}
  { }

  // check condition only on scope exit
  // https://en.cppreference.com/w/cpp/error/uncaught_exception
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4152.pdf
  ~Ensure() {
    if constexpr (g_build_mode == BuildMode::Dev || g_build_mode == BuildMode::Test || g_build_mode == BuildMode::Qa) {
      if (!_condition_function()) [[unlikely]] {
        try {
          std::shared_ptr<Record> record_failed =
              get_event_record(_source_location, EventCategory::Contract, Event::Ensure, UUID());
          record_failed->insert_or_assign("status"s, "Failed");
          sink::g_sink_manager.write_record(record_failed);
          sink::g_sink_manager.flush();   // throw could terminate program

          const std::string message =
              gformat("ERROR: {}: ensure condition failed on exit",
                      format_source_location(_source_location));
          throw contract_violation{message};
        } catch (...) {
          if (std::uncaught_exceptions() == _uncaught_exceptions) {
            throw;   // safe to rethrow
          } else {
            // unsafe to throw an exception - discard
          }
        }
      } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
        std::shared_ptr<Record> record_passed =
            get_event_record(_source_location, EventCategory::Contract, Event::Ensure, UUID());
        record_passed->insert_or_assign("status"s, "Passed");
        sink::g_sink_manager.write_record(record_passed);
      }
    }
  }

 private:
  int _uncaught_exceptions;
  std::function<bool()> _condition_function;
  source_location _source_location;
};

// -----------------------------------------------------------------------------
// restore diagnostic settings
#if defined(GIOPLER_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif defined(GIOPLER_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
namespace giopler::prod
{

// -----------------------------------------------------------------------------
/// confirms a condition that should be satisfied where it appears in a function body
// logs the error and throws exception
// this contract check is always enabled when the library is enabled, even in production mode
void certify([[maybe_unused]] const bool condition,
             [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    if (!condition) [[unlikely]] {
      std::shared_ptr<Record> record_failed =
          get_event_record(source_location, EventCategory::Contract, Event::Certify, UUID());
          record_failed->insert_or_assign("status"s, "Failed");
      sink::g_sink_manager.write_record(record_failed);
      sink::g_sink_manager.flush();   // throw could terminate program

      const std::string message =
          gformat("ERROR: {}: certify failed",
                  format_source_location(source_location));
      throw contract_violation{message};
    } else if constexpr (g_build_mode == BuildMode::Dev) {   // condition was met, but in Dev mode - send tracing event
      std::shared_ptr<Record> record_passed =
          get_event_record(source_location, EventCategory::Contract, Event::Certify, UUID());
      record_passed->insert_or_assign("status"s, "Passed");
      sink::g_sink_manager.write_record(record_passed);
    }
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::prod

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_CONTRACT_HPP
