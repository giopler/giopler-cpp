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
#ifndef GIOPLER_TRACE_HPP
#define GIOPLER_TRACE_HPP

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
/// log executing code on a certain line in the program
// used for debugging purposes
void line([[maybe_unused]] const std::string_view message = ""sv,
          [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Trace, Event::Line,
                         UUID::get_nil(), 0, message);
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
/// log executing code on a certain line in the program
// used for debugging purposes
void line([[maybe_unused]] StringFunction auto message_function,
          [[maybe_unused]] const source_location& source_location = source_location::current())
{
  if constexpr (g_build_mode == BuildMode::Dev) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Trace, Event::Line,
                         UUID::get_nil(), 0, message_function());
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

// -----------------------------------------------------------------------------
/// sets a runtime breakpoint in the program
// used for debugging purposes
// forces the function to always be inlined
//
// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
// https://clang.llvm.org/docs/AttributeReference.html
// https://docs.microsoft.com/en-us/cpp/cpp/attributes
//
// setting a breakpoint for different compilers and processors
// https://stackoverflow.com/a/49079078
// https://github.com/nemequ/portable-snippets/blob/master/debug-trap/debug-trap.h
// https://github.com/scottt/debugbreak/blob/master/debugbreak.h
// https://clang.llvm.org/docs/LanguageExtensions.html
// https://llvm.org/docs/LangRef.html#llvm-debugtrap-intrinsic
// https://docs.microsoft.com/en-us/cpp/intrinsics/debugbreak
// https://gitlab.gnome.org/GNOME/glib/-/blob/main/glib/gbacktrace.h
// https://web.archive.org/web/20210114140648/https://processors.wiki.ti.com/index.php/Software_Breakpoints_in_the_IDE
[[gnu::always_inline]][[clang::always_inline]][[msvc::forceinline]] inline
void set_breakpoint()
{
  if constexpr (g_build_mode == BuildMode::Dev) {
#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
      __builtin_debugtrap();   // CLang
#elif defined(__has_builtin) && __has_builtin(__debugbreak)
      __debugbreak();
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
      __debugbreak();   // Microsoft C Compiler and Intel Compiler Collection
#elif defined(__i386__) || defined(__x86_64__)
      __asm__ __volatile__("int3");   // x86/x86_64 processors
#else
#error Unsupported platform or compiler
#endif
  }
}

// -----------------------------------------------------------------------------
// restore diagnostic settings
#pragma GCC diagnostic pop

// -----------------------------------------------------------------------------
}   // namespace gioppler::dev

// -----------------------------------------------------------------------------
namespace giopler::prod
{

// -----------------------------------------------------------------------------
/// documents
void branch([[maybe_unused]] const std::string_view message = ""sv,
            [[maybe_unused]] const giopler::source_location& source_location = giopler::source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Trace, Event::Branch,
                         UUID::get_nil(), 0, message);
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
void branch([[maybe_unused]] StringFunction auto message_function,
            [[maybe_unused]] const giopler::source_location& source_location = giopler::source_location::current())
{
  if constexpr (g_build_mode != BuildMode::Off) {
    std::shared_ptr<Record> record =
        get_event_record(source_location, EventCategory::Trace, Event::Branch,
                         UUID::get_nil(), 0, message_function());
    sink::g_sink_manager.write_record(record);
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::prod

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_TRACE_HPP
