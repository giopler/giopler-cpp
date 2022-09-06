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
#ifndef GIOPPLER_GIOPPLER_HPP
#define GIOPPLER_GIOPPLER_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <version>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>

// -----------------------------------------------------------------------------
namespace gioppler {

// ---------------------------------------------------------------------------
class ProfileData {
 public:
  explicit ProfileData(const std::string_view parent_function_signature,
                       const std::string_view function_signature)
      :
      _parent_function_signature(parent_function_signature),
      _function_signature(function_signature),
      _function_calls(),
      _sum_of_count(),
      _linux_event_data_total(),
      _linux_event_data_self() {
  }

  static void write_header(std::ostream &os) {
    os << "Subsystem,ParentFunction,Function,Calls,Count,";
    LinuxEventsData::write_header(os);
  }

  void write_data(std::ostream &os) {

  }

  ProfileData &operator+=(const ProfileData &rhs) {
    _sum_of_count += rhs._sum_of_count;
    _linux_event_data_total += rhs._linux_event_data_total;
    _linux_event_data_self += rhs._linux_event_data_self;
    return *this;
  }

  friend ProfileData operator+(ProfileData lhs, const ProfileData &rhs) {
    lhs += rhs;
    return lhs;
  }

  ProfileData &operator-=(const ProfileData &rhs) {
    _sum_of_count -= rhs._sum_of_count;
    _linux_event_data_total -= rhs._linux_event_data_total;
    _linux_event_data_self -= rhs._linux_event_data_self;
    return *this;
  }

  friend ProfileData operator-(ProfileData lhs, const ProfileData &rhs) {
    lhs -= rhs;
    return lhs;
  }

 private:
  const std::string_view _parent_function_signature;
  const std::string_view _function_signature;
  uint64_t _function_calls;
  double _sum_of_count;
  LinuxEventsData _linux_event_data_total;
  LinuxEventsData _linux_event_data_self;
};

// -----------------------------------------------------------------------------
template<BuildMode build_mode = g_build_mode>
class Function {
 public:
  Function([[maybe_unused]] const std::string_view subsystem = "",
           [[maybe_unused]] const double count = 0.0,
           [[maybe_unused]] std::string session = "",
           [[maybe_unused]] const source_location &location =
           source_location::current())
  requires (build_mode == BuildMode::off) {
  }

  Function(const std::string_view subsystem = "",
           const double count = 0.0,
           std::string session = "",
           const source_location &location = source_location::current())
  requires (build_mode == BuildMode::profile) {
    check_create_program_thread();
  }

  ~Function() {
    check_destroy_program_thread();
  }

 private:
  using ProfileKey = std::pair<std::string_view, std::string_view>;
  static std::unordered_map<ProfileKey, ProfileData> _profile_map;
  static std::mutex _map_mutex;

  static thread_local inline std::stack<Function<build_mode>>  _functions;
  static thread_local inline std::stack<std::string>  _subsystems;
  static thread_local inline std::stack<std::string>  _sessions;

  // all accesses require modifying data
  // no advantage to use a readers-writer lock (a.k.a. shared_mutex)
  static void upsert_profile_map(const ProfileData &profile_record) {

  }

  void write_profile_map() {
    // sort descending by key
    std::multimap<double, ProfileKey, std::greater<>> sorted_profiles;
    sorted_profiles.reserve(_profile_map.size());

    // or sorted_profiles.copy(_profile_map.keys())
    //    sorted_profiles.sort()

    for (const auto &profile : _profile_map) {
      sorted_profiles.push_back(profile.first);
    }

    for (const auto &profile : sorted_profiles) {
      // std::cout << _profile_map[profile] << ' ';
    }
  }

  void check_create_program_thread() {
    Program::check_create();
    Thread::check_create();
  }

  void check_destroy_program_thread() {
    if (_functions.empty()) {
      Thread::destroy();
    }
    if (Thread::all_threads_done()) {
      write_profile_map();
      Program::check_destroy();
    }
  }
};

}   // namespace gioppler

#endif // defined GIOPPLER_GIOPPLER_HPP
