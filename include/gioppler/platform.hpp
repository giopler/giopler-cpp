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
#ifndef GIOPPLER_PLATFORM_HPP
#define GIOPPLER_PLATFORM_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <string>
#include <cstdint>

#include "gioppler/config.hpp"

// -----------------------------------------------------------------------------
namespace gioppler {
// these values are assumed to be constant for the duration of the program execution
extern uint64_t get_memory_page_size();
extern uint64_t get_physical_memory();
extern uint64_t get_total_cpu_cores();
extern uint64_t get_available_cpu_cores();
extern std::string get_program_name();
extern uint64_t get_process_id();
extern std::string get_real_username();
extern std::string get_effective_username();
extern std::string get_architecture();

// these values could change as the program runs
extern uint64_t get_thread_id();
extern uint64_t get_node_id();
extern uint64_t get_cpu_id();
extern uint64_t get_available_memory();
}   // namespace gioppler

// -----------------------------------------------------------------------------
#if defined(GIOPPLER_PLATFORM_LINUX)
#include "gioppler/linux/platform.hpp"
#endif

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_PLATFORM_HPP
