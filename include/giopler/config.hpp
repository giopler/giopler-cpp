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
#ifndef GIOPLER_CONFIG_HPP
#define GIOPLER_CONFIG_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <string>
#include <string_view>
#include <cstdint>
using namespace std::literals;

// -----------------------------------------------------------------------------
namespace giopler {

// -----------------------------------------------------------------------------
// g_build_mode controls the operating mode for the library.
// Normally this is controlled from the CMake build files.
// Define one of these constants to manually control the variable.
enum class BuildMode {Off, Dev, Test, Bench, Prof, Qa, Prod};
#if defined(GIOPLER_BUILD_MODE_OFF)
constexpr static inline BuildMode g_build_mode{BuildMode::Off};
#elif defined(GIOPLER_BUILD_MODE_DEV)
constexpr static inline BuildMode g_build_mode{BuildMode::Dev};
#elif defined(GIOPLER_BUILD_MODE_TEST)
constexpr static inline BuildMode g_build_mode{BuildMode::Test};
#elif defined(GIOPLER_BUILD_MODE_BENCH)
constexpr static inline BuildMode g_build_mode{BuildMode::Bench};
#elif defined(GIOPLER_BUILD_MODE_PROF)
constexpr static inline BuildMode g_build_mode{BuildMode::Prof};
#elif defined(GIOPLER_BUILD_MODE_QA)
constexpr static inline BuildMode g_build_mode{BuildMode::Qa};
#elif defined(GIOPLER_BUILD_MODE_PROD)
constexpr static inline BuildMode g_build_mode{BuildMode::Prod};
#else
#warning Build mode not defined. Disabling Giopler library.
constexpr static inline BuildMode g_build_mode{BuildMode::Off};
#endif

// -----------------------------------------------------------------------------
/// convert the build mode enum value into a string
constexpr std::string_view get_build_mode_name() {
  switch (g_build_mode) {
    case BuildMode::Off:    return "Off"sv;
    case BuildMode::Dev:    return "Dev"sv;
    case BuildMode::Test:   return "Test"sv;
    case BuildMode::Bench:  return "Bench"sv;
    case BuildMode::Prof:   return "Prof"sv;
    case BuildMode::Qa:     return "Qa"sv;
    case BuildMode::Prod:   return "Prod"sv;
  }
  return "Unknown"sv;
}

// -----------------------------------------------------------------------------
// g_compiler is the name of the compiler
enum class Compiler {Unknown, Gcc, Clang, Microsoft, Intel};
#if defined(__clang__)
constexpr static inline Compiler g_compiler{Compiler::Clang};
#define GIOPLER_COMPILER_CLANG 1
#elif defined(__GNUC__)
constexpr static inline Compiler g_compiler{Compiler::Gcc};
#define GIOPLER_COMPILER_GCC 1
#elif defined(_MSC_VER)
constexpr static inline Compiler g_compiler{Compiler::Microsoft};
#define GIOPLER_COMPILER_MICROSOFT 1
#else
#warning Compiler not recognized
constexpr static inline Compiler g_compiler{Compiler::Unknown};
#endif

// -----------------------------------------------------------------------------
/// convert the compiler enum value into a string
constexpr std::string_view get_compiler_name() {
  switch (g_compiler) {
    case Compiler::Unknown:   return "Unknown:"sv;
    case Compiler::Gcc:       return "Gcc"sv;
    case Compiler::Clang:     return "Clang"sv;
    case Compiler::Microsoft: return "Microsoft"sv;
    case Compiler::Intel:     return "Intel"sv;
  }
  return "Unknown"sv;
}

// -----------------------------------------------------------------------------
/// Platform defines the operating system.
// Often used to control include files, so define constants also.
enum class Platform {Linux, Windows, Bsd};
#if defined(__linux__) || defined(__ANDROID__)
#define GIOPLER_PLATFORM_LINUX 1
constexpr static inline Platform g_platform = Platform::Linux;
#elif defined(_WIN32) || defined(_WIN64)
#define GIOPLER_PLATFORM_WINDOWS 1
constexpr static inline Platform g_platform = Platform::Windows;
#elif defined(BSD) || defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__)
#define GIOPLER_PLATFORM_BSD 1
constexpr static inline Platform g_platform = Platform::Bsd;
#else
#error Operating system platform unsupported.
#endif

// -----------------------------------------------------------------------------
/// convert the platform enum value into a string
constexpr std::string_view get_platform_name() {
  switch (g_platform) {
    case Platform::Linux:     return "Linux"sv;
    case Platform::Windows:   return "Windows"sv;
    case Platform::Bsd:       return "Bsd"sv;
  }
  return "Unknown"sv;
}

// -----------------------------------------------------------------------------
/// CPU architecture.
enum class Architecture {X86, Arm, Unknown};
#if defined(__i386__) || defined(__x86_64__)
constexpr static inline Architecture g_architecture = Architecture::X86;
#elif defined(__arm__) || defined(__arm64__) || defined(__aarch64__)
constexpr static inline Architecture g_architecture = Architecture::Arm;
#else
#warning Could not identify CPU architecture.
constexpr static inline Architecture g_architecture = Architecture::Unknown;
#endif

// -----------------------------------------------------------------------------
}   // namespace giopler

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_CONFIG_HPP
