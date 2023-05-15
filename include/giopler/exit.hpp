// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

#pragma once
#ifndef GIOPLER_EXIT_HPP
#define GIOPLER_EXIT_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cstdlib>
#include <exception>
#include <mutex>

#include "giopler/sink.hpp"

// -----------------------------------------------------------------------------
namespace giopler
{

// -----------------------------------------------------------------------------
/// function to call on program exit
// https://en.cppreference.com/w/cpp/utility/program/exit
// https://en.cppreference.com/w/cpp/utility/program/quick_exit
// https://en.cppreference.com/w/cpp/error/terminate
// https://en.cppreference.com/w/cpp/utility/program/signal

// -----------------------------------------------------------------------------
class ExitFunction {
 public:
  ExitFunction() {
    // called on normal program termination
    std::atexit(atexit_function);

    // normal program termination without completely cleaning resource
    std::at_quick_exit(quick_exit_function);

    // called by the C++ runtime when the program cannot continue
    _previous_terminate_handler = std::set_terminate(terminate_function);
  }

  /// called on normal program termination
  static void atexit_function() {
    // write out logging data from each thread
    sink::g_sink_manager.flush();
  }

  /// normal program termination without completely cleaning resource
  static void quick_exit_function() {
    // write out logging data from each thread
    sink::g_sink_manager.flush();
  }

  /// called by the C++ runtime when the program cannot continue
  static void terminate_function() {
    // write out logging data from each thread
    sink::g_sink_manager.flush();

    if (_previous_terminate_handler) {
      _previous_terminate_handler();
    }
  }

 private:
  static inline std::terminate_handler _previous_terminate_handler;
};

// -----------------------------------------------------------------------------
static inline ExitFunction g_exit_function;

// -----------------------------------------------------------------------------
}   // namespace giopler

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_EXIT_HPP
