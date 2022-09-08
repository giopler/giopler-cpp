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
#ifndef GIOPPLER_PROFILE_HPP
#define GIOPPLER_PROFILE_HPP

#if __cplusplus < 202002L
#error C++20 or newer support required to use this header-only library.
#endif

#include <memory>
#include <string>

// -----------------------------------------------------------------------------
namespace gioppler::dev {

// -----------------------------------------------------------------------------
extern Record read_event_counters();

// -----------------------------------------------------------------------------
/// target += other
// assumes all entries are numeric (Integer or Real)
// assumes the other Record contains all key values
void add_number_record(Record& target, const Record& other) {
  for (const auto& [key, value] : other) {
    if (value.get_type() == RecordValue::Type::Integer) {
      if (!target.contains(key)) {
        target.insert({key, 0});
      }
      target.at(key) = target.at(key).get_integer() + value.get_integer();
    } else if (value.get_type() == RecordValue::Type::Real) {
      if (!target.contains(key)) {
        target.insert({key, 0.0});
      }
      target.at(key) = target.at(key).get_real() + value.get_real();
    } else {
      assert(false);
    }
  }
}

// -----------------------------------------------------------------------------
/// target -= other
// assumes all entries are numeric (Integer or Real)
// assumes the other Record contains all key values
// clamps values at zero - does not go negative
void subtract_number_record(Record& target, const Record& other) {
  for (const auto& [key, value] : other) {
    if (value.get_type() == RecordValue::Type::Integer) {
      if (!target.contains(key)) {
        target.insert({key, 0});
      }
      if (target.at(key).get_integer() <= value.get_integer()) {
        target.at(key) = 0;
      } else {
        target.at(key) = target.at(key).get_integer() - value.get_integer();
      }
    } else if (value.get_type() == RecordValue::Type::Real) {
      if (!target.contains(key)) {
        target.insert({key, 0.0});
      }
      if (target.at(key).get_real() <= value.get_real()) {
        target.at(key) = 0.0;
      } else {
        target.at(key) = target.at(key).get_real() - value.get_real();
      }
    } else {
      assert(false);
    }
  }
}

// -----------------------------------------------------------------------------
class Function {
 public:
  Function([[maybe_unused]] const double workload = 0,
           [[maybe_unused]] gioppler::source_location source_location = gioppler::source_location::current())
  {
    if constexpr (g_build_mode == BuildMode::Dev) {
      std::shared_ptr<Record> record = std::make_shared<Record>(
          create_event_record(source_location, "trace"sv, "function"sv));
      sink::g_sink_manager.write_record(record);
    } else if constexpr (g_build_mode == BuildMode::Prof) {
      _source_location         = std::make_unique<gioppler::source_location>(source_location);
      _event_counters_start    = std::make_unique<Record>(read_event_counters());
      _event_counters_children = std::make_unique<Record>();
    }
  }

  ~Function() {
    if constexpr (g_build_mode == BuildMode::Prof) {
      Record event_counters_end{read_event_counters()};

      std::shared_ptr<Record> record_total = std::make_shared<Record>(
          create_event_record(*_source_location, "profile"sv, "function_total"sv));
      std::shared_ptr<Record> record_self = std::make_shared<Record>(record_total);
      (*record_self)["evt.event"s] = "function_self"s;

      subtract_number_record(event_counters_end, *_event_counters_start);
      record_total->merge(event_counters_end);
      sink::g_sink_manager.write_record(record_total);

      subtract_number_record(event_counters_end, *_event_counters_children);
      record_self->merge(event_counters_end);
      sink::g_sink_manager.write_record(record_self);
    }
  }

  void track_child(const Record& child_record) {
    add_number_record(*_event_counters_children, child_record);
  }

 private:
  std::unique_ptr<gioppler::source_location> _source_location;
  // use shared pointers to minimize their cost if build mode disables the class
  std::unique_ptr<Record> _event_counters_start;
  std::unique_ptr<Record> _event_counters_children;
};


// -----------------------------------------------------------------------------
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

}   // namespace gioppler::dev

#endif // defined GIOPPLER_PROFILE_HPP
