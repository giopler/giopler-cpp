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
#ifndef GIOPLER_PLATFORM_HPP
#define GIOPLER_PLATFORM_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <string>
#include <cstdint>

#include "giopler/config.hpp"

// -----------------------------------------------------------------------------
namespace giopler {
// these values are assumed to be constant for the duration of the program execution
extern uint64_t get_memory_page_size();
extern uint64_t get_physical_memory();
extern uint64_t get_total_cpu_cores();
extern uint64_t get_available_cpu_cores();
extern std::string get_program_name();
extern uint64_t get_process_id();
extern std::string get_architecture();
extern std::string get_host_name();
extern std::string get_real_username();
extern std::string get_effective_username();

// these values could change as the program runs
extern uint64_t get_thread_id();
extern uint64_t get_node_id();
extern uint64_t get_cpu_id();
extern uint64_t get_available_memory();
}   // namespace giopler

// -----------------------------------------------------------------------------
#if defined(GIOPLER_PLATFORM_LINUX)
#include "giopler/linux/platform.hpp"
#endif

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_PLATFORM_HPP
