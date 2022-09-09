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
