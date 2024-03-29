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
#ifndef GIOPLER_UTILITY_HPP
#define GIOPLER_UTILITY_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <random>
#include <random>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <syncstream>
#include <unordered_map>
#include <utility>
#include <variant>
using namespace std::literals;

#include "giopler/config.hpp"
#include "giopler/platform.hpp"
#include "giopler/pcg.hpp"

// -----------------------------------------------------------------------------
/// String formatting function
#if defined(__cpp_lib_format)
// https://en.cppreference.com/w/cpp/utility/format
// CLion reports an invalid error here when using Gcc
// https://youtrack.jetbrains.com/issue/CPP-33258/stdformat-reports-error
#include <format>
namespace giopler {
template <typename... Args>
[[nodiscard]] std::string gformat(std::string_view fmt, Args&&... args) {
  return std::vformat(fmt, std::make_format_args(args...));
}
}   // namespace giopler
#else
// https://github.com/fmtlib/fmt
#define FMT_HEADER_ONLY 1
#include <fmt/chrono.h>
namespace giopler {
template <typename... T>
[[nodiscard]] std::string gformat(std::string_view fmt, T&&... args) {
  return fmt::vformat(fmt, fmt::make_format_args(args...));
}
}   // namespace giopler
#endif

// -----------------------------------------------------------------------------
namespace giopler {

// -----------------------------------------------------------------------------
/// used to combine multiple hash values
// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
// https://en.cppreference.com/w/cpp/utility/hash
template<typename T, typename... Rest>
void hash_combine(std::size_t &seed, const T &v, const Rest &... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hash_combine(seed, rest), ...);
}

// -----------------------------------------------------------------------------
/// simplify creating shared pointers to unordered maps
// forces the use of an initialization list constructor
// without this we get an ambiguous overload error
// example: auto foo = make_shared_init_list<std::map<double,std::string>>({{1000, "hi"}});
// https://stackoverflow.com/questions/36445642/initialising-stdshared-ptrstdmap-using-braced-init
// https://stackoverflow.com/a/36446143/4560224
// https://en.cppreference.com/w/cpp/utility/initializer_list
// https://en.cppreference.com/w/cpp/language/list_initialization
template<class Container>
std::shared_ptr<Container>
make_shared_init_list(std::initializer_list<typename Container::value_type> il) {
    return std::make_shared<Container>(il);
}

// -----------------------------------------------------------------------------
/// Use the environment to resolve the location of the home directory.
std::filesystem::path get_home_path() {
  if (std::getenv("HOME")) {
    return std::getenv("HOME");
  } else if (std::getenv("HOMEDRIVE") && std::getenv("HOMEPATH")) {
    return std::filesystem::path(std::getenv("HOMEDRIVE")) += std::getenv("HOMEPATH");
  } else if (std::getenv("USERPROFILE")) {
    return std::getenv("USERPROFILE");
  } else {
    return "";
  }
}

// -----------------------------------------------------------------------------
/// Resolve macros, canonicalize, and create directory.
std::filesystem::path resolve_directory(const std::string_view directory) {
  std::filesystem::path directory_path;
  std::string_view rest;
  if (directory.starts_with("<temp>")) {
    rest = directory.substr("<temp>"sv.size());
    directory_path = std::filesystem::temp_directory_path();
  } else if (directory.starts_with("<home>")) {
    rest = directory.substr("<home>"sv.size());
    directory_path = get_home_path();
  } else if (directory.starts_with("<current>")) {
    rest = directory.substr("<current>"sv.size());
    directory_path = std::filesystem::current_path();
  } else if (directory.empty()) {
    directory_path = std::filesystem::current_path();
  } else {   // otherwise use path as is
    rest = directory;
  }

  directory_path += rest;
  directory_path = std::filesystem::weakly_canonical(directory_path);
  std::filesystem::create_directories(directory_path);
  return directory_path;
}

// -----------------------------------------------------------------------------
/// Create file path for sink destination.
std::filesystem::path create_filename(const std::string_view extension = "txt") {
  std::random_device random_device;
  std::independent_bits_engine<std::default_random_engine, 32, std::uint_least32_t>
    generator{random_device()};

  const std::string program_name{get_program_name()};
  const uint64_t process_id{get_process_id()};
  const uint32_t salt{generator() % 10'000};   // up to four digits
  const std::string extension_dot = (!extension.empty() && extension[0] != '.') ? "." : "";
  const std::string filename{gformat("{}-{}-{}{}{}", program_name, process_id, salt, extension_dot, extension)};
  return filename;
}

// -----------------------------------------------------------------------------
/// Returns an open output stream for the given path and file extension.
// The stream does not require synchronization to use.
// https://en.cppreference.com/w/cpp/io/basic_ios/rdbuf
// https://en.cppreference.com/w/cpp/io/basic_ostream/basic_ostream
// Note: the created std::ostream does not take ownership of the streambuf passed in
// Directory patterns:
//   <temp>, <current>, <home>   - optionally follow these with other directories
//   <cout>, <clog>, <cerr>      - these specify the entire path
std::unique_ptr<std::ostream>
get_output_filepath(const std::string_view directory = "<temp>"sv, const std::string_view extension = "txt"sv)
{
  if (directory == "<cerr>") {
    std::clog << "INFO: giopler: adding log destination: cerr" << std::endl;
    return std::make_unique<std::ostream>(std::cerr.rdbuf());
  } else if (directory == "<cout>") {
    std::clog << "INFO: giopler: adding log destination: cout" << std::endl;
    return std::make_unique<std::ostream>(std::cout.rdbuf());
  } else if (directory == "<clog>") {
    std::clog << "INFO: giopler: adding log destination: clog" << std::endl;
    return std::make_unique<std::ostream>(std::clog.rdbuf());
  }

  const std::filesystem::path directory_path = resolve_directory(directory);
  const std::filesystem::path filename_path  = create_filename(extension);
  const std::filesystem::path full_path      = directory_path/filename_path;
  std::clog << "INFO: giopler: adding log destination: " << full_path << std::endl;
  return std::make_unique<std::ofstream>(full_path, std::ios::trunc);
}

// -----------------------------------------------------------------------------
// we need to use system_clock for reporting the start time
// we need to use steady_clock for reporting time deltas
using TimestampSystem = std::chrono::system_clock::time_point;
using TimestampSteady = std::chrono::steady_clock::time_point;

// -----------------------------------------------------------------------------
/// returns the current timestamp
TimestampSystem now_system() {
  return std::chrono::system_clock::now();
}

// -----------------------------------------------------------------------------
/// returns the current timestamp
TimestampSteady now_steady() {
  return std::chrono::steady_clock::now();
}

// -----------------------------------------------------------------------------
static inline const TimestampSystem start_system_time = std::chrono::system_clock::now();
static inline const TimestampSteady start_steady_time = std::chrono::steady_clock::now();

// -----------------------------------------------------------------------------
/// convert the given Timestamp into nanoseconds
constexpr std::uint64_t to_nanoseconds(const TimestampSystem ts) {
  const std::uint64_t timestamp_ns =
    std::chrono::duration_cast<std::chrono::nanoseconds>(ts.time_since_epoch()).count();
  return timestamp_ns;
}

// -----------------------------------------------------------------------------
/// convert the given Timestamp into nanoseconds
constexpr std::uint64_t to_nanoseconds(const TimestampSteady ts) {
  const std::uint64_t timestamp_ns =
    std::chrono::duration_cast<std::chrono::nanoseconds>(ts.time_since_epoch()).count();
  return timestamp_ns;
}

// -----------------------------------------------------------------------------
/// convert nanosecond counter to floating point seconds
constexpr double ns_to_sec(const std::uint64_t ns) {
  constexpr double factor = 0.000'000'001;
  return static_cast<double>(ns) * factor;
}

// -----------------------------------------------------------------------------
/// convert the given Timestamp into seconds
constexpr double to_seconds(const TimestampSteady ts) {
  return ns_to_sec(to_nanoseconds(ts));
}

// -----------------------------------------------------------------------------
/// return seconds in double between the two timestamps
constexpr double timestamp_diff(const TimestampSteady start, const TimestampSteady end) {
  const std::uint64_t timestamp_start_ns = to_nanoseconds(start);
  const std::uint64_t timestamp_end_ns   = to_nanoseconds(end);
  const std::uint64_t delta_ns           = timestamp_end_ns - timestamp_start_ns;
  return static_cast<double>(delta_ns) / 1'000'000'000.0;
}

// -----------------------------------------------------------------------------
/// returns seconds since the program started running
double get_time_delta() {
  return timestamp_diff(start_steady_time, now_steady());
}

// -----------------------------------------------------------------------------
/// Convert a time point into ISO-8601 string format.
// https://en.wikipedia.org/wiki/ISO_8601
// https://www.iso.org/obp/ui/#iso:std:iso:8601:-1:ed-1:v1:en
// https://www.iso.org/obp/ui/#iso:std:iso:8601:-2:ed-1:v1:en
// https://en.cppreference.com/w/cpp/chrono/system_clock/formatter
// https://en.cppreference.com/w/cpp/chrono/utc_clock/formatter
// Note: C++20 utc_clock is not quite implemented yet for gcc.
// Parameter example: const auto start = std::chrono::system_clock::now();
std::string format_timestamp(const TimestampSystem ts)
{
  // support for %Ez was merged into libfmt on December 10, 2022
  // https://github.com/fmtlib/fmt/issues/3220
  // current released version is 9.1.0 from August 2022
  const std::string tz = gformat("{0:%z}", ts);   // "-0700"
  const std::string colon_tz = tz.substr(0, 3) + ":" + tz.substr(3, 2);

  return gformat("{0:%FT%T}{1}", ts, colon_tz);
}

// -----------------------------------------------------------------------------
/// Implement C++20 standard source_location
// uses the __builtin... intrinsic functions provided by the compilers
// CLang++ 14 still has non-standard source_location support
// Note: g++ does not support __builtin_COLUMN()
// https://en.cppreference.com/w/cpp/utility/source_location
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html#source-location-builtins
// https://github.com/microsoft/STL/issues/54
// Note: a bug in CLang++ appears to be getting in the way
// https://stackoverflow.com/questions/64762278/decltypefun-for-consteval-methods
// https://bugs.llvm.org/show_bug.cgi?id=47350
// https://open-std.org/JTC1/SC22/WG21/docs/papers/2020/p1937r2.html
class source_location
{
public:
  constexpr source_location(const char* file,
                            const char* function,
                            const int line)
    : file_name_(file),
      function_name_(function),
      line_(line)
  {}

  source_location(const source_location& other) = default;
  source_location(source_location&& other) noexcept = default;

  static constexpr source_location
  current(const char* builtin_file = __builtin_FILE(),
          const char* builtin_function = __builtin_FUNCTION(),
          const int builtin_line = __builtin_LINE())
  {
    return {builtin_file, builtin_function, builtin_line};
  }

  [[nodiscard]] constexpr const char* file_name() const {
    return file_name_;
  }

  [[nodiscard]] constexpr const char* function_name() const {
    return function_name_;
  }

  [[nodiscard]] constexpr int line() const {
    return line_;
  }

 private:
  const char* file_name_;
  const char* function_name_;
  const int line_;
};

// -----------------------------------------------------------------------------
/// convert a source_location into a string
// useful when creating the message when throwing an exception
std::string format_source_location(const source_location &location)
{
    std::string message =
      gformat("{}({}): {}",
             location.file_name(),
             location.line(),
             location.function_name());
    return message;
}

// -----------------------------------------------------------------------------
/// make UUID type safe
class UUID
{
 public:
    UUID() : value_{get_uuid()} { }

    explicit UUID(std::string_view uuid_string) : value_{uuid_string} { }

    [[nodiscard]] std::string get_string() const { return value_; }

    static UUID get_nil() {
      return UUID{"00000000-0000-4000-8000-000000000000"s};
    }

 private:
  std::string value_;

  // ---------------------------------------------------------------------------
  // Generator Version 4 (random) Variant 1 (RFC 4122/DCE 1.1) UUIDs from random values
  // can produce about 7 million UUIDs per second (per CPU core)
  // https://stackoverflow.com/questions/24365331/how-can-i-generate-uuid-in-c-without-using-boost-library
  // https://datatracker.ietf.org/doc/html/rfc4122
  // https://en.wikipedia.org/wiki/Universally_unique_identifier
  // cat /proc/sys/kernel/random/uuid
  // sample: 8e08aa0f-8024-4e31-8400-e75cd1217239
  static std::string get_uuid()
  {
      static constexpr auto UUID_LEN = 36;   // 5c47ba38-3e2b-48b1-988e-f16921213939
      static const char hex_char[] =
        {'0', '1',  '2',  '3',  '4',  '5', '6',  '7',
         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
      static thread_local pcg                               gen;
      static thread_local std::uniform_int_distribution<> hex_digit(0, 15);
      static thread_local std::uniform_int_distribution<>   variant(8, 11);

      std::string result;
      result.reserve(UUID_LEN);
      for (int i = 0; i < 8; ++i) {
        result.push_back(hex_char[hex_digit(gen)]);
      }
      result.push_back('-');
      for (int i = 0; i < 4; ++i) {
        result.push_back(hex_char[hex_digit(gen)]);
      }
      result.push_back('-');
      result.push_back('4');
      for (int i = 0; i < 3; ++i) {
        result.push_back(hex_char[hex_digit(gen)]);
      }
      result.push_back('-');
      result.push_back(hex_char[variant(gen)]);
      for (int i = 0; i < 3; ++i) {
        result.push_back(hex_char[hex_digit(gen)]);
      }
      result.push_back('-');
      for (int i = 0; i < 12; ++i) {
        result.push_back(hex_char[hex_digit(gen)]);
      };
      return result;
  }
};

// -----------------------------------------------------------------------------
namespace prod {

// -----------------------------------------------------------------------------
std::string uuid() {
  return UUID().get_string();
}

// -----------------------------------------------------------------------------
}   // namespace prod

// -----------------------------------------------------------------------------
/// helper to create a string hash from a string
std::string hash_string(std::string_view id) {
  std::hash<std::string_view> hash;
  return std::to_string(hash(id));
}

// -----------------------------------------------------------------------------
/// typesafe declaration for a function without parameters that returns a boolean
// Usage:
//   fn(BoolFunction auto input_bool_func)
//   bool input_bool_func()
template <typename F>
concept BoolFunction = requires (F f) {
    requires std::regular_invocable<F>;
    {f()} -> std::convertible_to<bool>;
};

// -----------------------------------------------------------------------------
/// typesafe declaration for a function without parameters that returns a string
// Usage:
//   fn(StringFunction auto input_string_func)
//   std::string input_string_func()
template <typename F>
concept StringFunction = requires (F f) {
    requires std::regular_invocable<F>;
    {f()} -> std::convertible_to<std::string>;
};

// -----------------------------------------------------------------------------
}   // namespace giopler

// -----------------------------------------------------------------------------
template<>
struct std::hash<giopler::TimestampSystem> {
  std::size_t operator()(const giopler::TimestampSystem& timestamp) const {
    std::hash<std::uint64_t> hash;
    const std::uint64_t timestamp_ns = giopler::to_nanoseconds(timestamp);
    return hash(timestamp_ns);
  }
};

// -----------------------------------------------------------------------------
// defined in std namespace, so it is used automatically
template<class T1, class T2>
struct std::hash<std::pair<T1, T2>> {
  std::size_t operator()(const std::pair<T1, T2>& pair) const {
    std::size_t seed = 0;
    giopler::hash_combine(seed, pair.first, pair.second);
    return seed;
  }
};

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_UTILITY_HPP
