// Copyright (c) 2022. Carlos Reyes
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
//

#pragma once
#ifndef GIOPLER_RECORD_HPP
#define GIOPLER_RECORD_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <source_location>
#include <syncstream>
#include <unordered_map>
#include <variant>
using namespace std::literals;

#include "giopler/config.hpp"
#include "giopler/utility.hpp"
#include "giopler/platform.hpp"

// -----------------------------------------------------------------------------
namespace giopler {

// -----------------------------------------------------------------------------
// Data Dictionary:

// these values are constant for the duration of the program run
// - prog.run_id                  - string    - generated at client; unique to program run (UUID)
// - prog.start_ts                - timestamp - when program started running
// - prog.memory_page_size        - integer   - memory page size (bytes)
// - prog.physical_memory         - integer   - physical memory size (bytes)
// - prog.total_cpu_cores         - integer   - total number of CPU cores
// - prog.available_cpu_cores     - integer   - number of available CPU cores
// - prog.program_name            - string    - system program name
// - prog.process_id              - integer   - system process id (/proc/sys/kernel/pid_max)
// - prog.build_mode              - string    - dev, test, prof, qa, prod
// - prog.compiler                - string    - Gcc, Clang, Microsoft, Intel
// - prog.platform                - string    - Linux, Windows, Bsd
// - prog.architecture            - string    - x86_64
// - prog.real_username           - string    - logged-in username
// - prog.effective_username      - string    - username that the process is running under

// these values could change as the program runs
// - evt.record_type              - string    - program, message, profile_linux
// - evt.event_category           - string    - contract, dev, prof, test, prod
// - evt.event                    - string    - uniquely identifies the event
// - attr.*                       - any       - user-defined attributes

// - loc.file                     - string    - source file name and path
// - loc.line                     - integer   - line number
// - loc.function                 - string    - function name and signature

// - trc.function                 - string    - last function traced via Function class
// - trc.parent_function          - string    - parent (calling) function traced via Function class

// - val.timestamp                - timestamp - when event occurred
// - val.thread_id                - integer   - system thread id
// - val.node_id                  - integer   - NUMA node id where thread is currently running
// - val.cpu_id                   - integer   - CPU core id where thread is currently running
// - val.available_memory         - integer   - free memory size (bytes)

// - val.message                  - string    - additional event details

// - prof.workload                - real      - user-assigned weight to profiled function calls (same for total and self)

// All of these have two event versions:
// - function_total               - sum of the function and other functions it calls
// - function_self                - sum of only the function, excluding other functions called

// - prof.duration                - real      - real (wall clock) duration (secs)

// Linux Performance Counters: (g_build_mode == BuildMode::Prof)

// - prof.sw.cpu_clock            - real      - CPU clock, a high-resolution per-CPU timer. (secs)
// - prof.sw.task_clock           - real      - clock count specific to the task that is running. (secs)
// - prof.sw.page_faults          - integer   - number of page faults
// - prof.sw.context_switches     - integer   - counts context switches
// - prof.sw.cpu_migrations       - integer   - number of times the process has migrated to a new CPU
// - prof.sw.page_faults_min      - integer   - number of minor page faults
// - prof.sw.page_faults_maj      - integer   - number of major page faults. These required disk I/O to handle
// - prof.sw.alignment_faults     - integer   - counts the number of alignment faults. Zero on x86
// - prof.sw.emulation_faults     - integer   - counts the number of emulation faults

// - prof.hw.cpu_cycles           - integer   - Total cycles
// - prof.hw.instructions         - integer   - Retired instructions (i.e., executed)

// - prof.hw.stall_cycles_front   - integer   - Stalled cycles during issue in the frontend
// - prof.hw.stall_cycles_back    - integer   - Stalled cycles during retirement in the backend

// - prof.hw.branch_instructions  - integer   - Retired branch instructions (i.e., executed)
// - prof.hw.branch_misses        - integer   - Mispredicted branch instructions

// - prof.hw.cache_references     - integer   - Cache accesses. Usually this indicates Last Level Cache accesses
// - prof.hw.cache_misses         - integer   - Cache misses. Usually this indicates Last Level Cache misses

// -----------------------------------------------------------------------------
/// replacement for std::variant; eventually might become a wrapper
// std::variant is currently broken
// Error: Implicitly defined constructor deleted due to variant member
// https://en.cppreference.com/w/cpp/utility/variant
// https://stackoverflow.com/questions/53310690/implicitly-defined-constructor-deleted-due-to-variant-member-n3690-n4140-vs-n46
// https://github.com/llvm/llvm-project/issues/39034
// https://stackoverflow.com/questions/65404305/why-is-the-defaulted-default-constructor-deleted-for-a-union-or-union-like-class
// https://stackoverflow.com/questions/63624014/why-does-clang-using-libstdc-delete-the-explicitly-defaulted-constructor-on
class RecordValue
{
 public:
  enum class Type {Empty, Boolean, Integer, Real, String, Timestamp};

  [[nodiscard]] Type get_type() const {
    return _record_value_type;
  }

  // ---------------------------------------------------------------------------
  RecordValue()
  : _record_value_type(Type::Empty) { }

  // ---------------------------------------------------------------------------
  /// create an empty record value matching type
  // useful when we need the default values
  explicit RecordValue(const Type type)
  : _record_value_type(type) { }

  // ---------------------------------------------------------------------------
  bool operator==(const RecordValue& record_value) const {
    if (_record_value_type == record_value._record_value_type) {
      switch (_record_value_type) {
        case Type::Empty:       return true;
        case Type::Boolean:     return _boolean_value   == record_value._boolean_value;
        case Type::Integer:     return _integer_value   == record_value._integer_value;
        case Type::Real:        return _real_value      == record_value._real_value;
        case Type::String:      return _string_value    == record_value._string_value;
        case Type::Timestamp:   return _timestamp_value == record_value._timestamp_value;
      }
    } else {
      return false;
    }
  }

  // ---------------------------------------------------------------------------
  RecordValue(const bool boolean_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Boolean), _boolean_value(boolean_value) { }

  [[nodiscard]] bool get_boolean() const {
    assert(_record_value_type == Type::Boolean);
    return _boolean_value;
  }

  void set_boolean(const bool boolean_value) {
    assert(_record_value_type == Type::Boolean);
    _boolean_value = boolean_value;
  }

  // ---------------------------------------------------------------------------
  RecordValue(const int64_t integer_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Integer), _integer_value(integer_value) { }

  RecordValue(const uint64_t integer_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Integer), _integer_value(static_cast<int64_t>(integer_value)) { }

  RecordValue(const uint32_t integer_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Integer), _integer_value(static_cast<int64_t>(integer_value)) { }

  RecordValue(const int32_t integer_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Integer), _integer_value(static_cast<int64_t>(integer_value)) { }

  [[nodiscard]] int64_t get_integer() const {
    assert(_record_value_type == Type::Integer);
    return _integer_value;
  }

  void set_integer(const int64_t int_value) {
    assert(_record_value_type == Type::Integer);
    _integer_value = int_value;
  }

  // ---------------------------------------------------------------------------
  RecordValue(const double real_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Real), _real_value(real_value) { }

  [[nodiscard]] double get_real() const {
    assert(_record_value_type == Type::Real);
    return _real_value;
  }

  void set_real(const double real_value) {
    assert(_record_value_type == Type::Real);
    _real_value = real_value;
  }

  // ---------------------------------------------------------------------------
  RecordValue(std::string_view string_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::String), _string_value(string_value) { }

  RecordValue(std::string string_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::String), _string_value(std::move(string_value)) { }

  RecordValue(const char* string_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::String), _string_value(string_value) { }

  [[nodiscard]] std::string get_string() const {
    assert(_record_value_type == Type::String);
    return _string_value;
  }

  void set_string(std::string_view string_value) {
    assert(_record_value_type == Type::String);
    _string_value = string_value;
  }

  // ---------------------------------------------------------------------------
  RecordValue(Timestamp timestamp_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Timestamp), _timestamp_value(timestamp_value) { }

  [[nodiscard]] Timestamp get_timestamp() const {
    assert(_record_value_type == Type::Timestamp);
    return _timestamp_value;
  }

  void set_timestamp(const Timestamp timestamp_value) {
    assert(_record_value_type == Type::Timestamp);
    _timestamp_value = timestamp_value;
  }

 private:
  friend struct std::hash<giopler::RecordValue>;
  Type _record_value_type;
  bool _boolean_value{};
  int64_t _integer_value{};
  double _real_value{};
  std::string _string_value{};
  Timestamp _timestamp_value{};
};

// -----------------------------------------------------------------------------
/// contents of the record catalog
// key type, key category, scale factor
using RecordCatalogInfo = std::tuple<RecordValue::Type, std::string, double>;

// -----------------------------------------------------------------------------
/// converting nanoseconds to seconds
// if scale factor != 1, then result type is double
constexpr static inline double ns_to_sec = 0.000'000'001;

// -----------------------------------------------------------------------------
/// catalog of valid keys and values for records
// [key name] = key info
using RecordCatalog = std::unordered_map<std::string, RecordCatalogInfo>;

// -----------------------------------------------------------------------------
/// Data being sent to a sink for processing.
using Record = std::unordered_map<std::string, RecordValue>;

// -----------------------------------------------------------------------------
/// initializer list for Record values
using RecordInit = std::initializer_list<Record::value_type>;

// -----------------------------------------------------------------------------
/// catalog of all valid Record keys
const RecordCatalog& get_record_catalog() {
  static const RecordCatalog record_catalog{
      {"prog.run_id"s,                        {RecordValue::Type::String,     "prog"s, 1}},
      {"prog.start_ts"s,                      {RecordValue::Type::Timestamp,  "prog"s, 1}},
      {"prog.memory_page_size"s,              {RecordValue::Type::Integer,    "prog"s, 1}},
      {"prog.physical_memory"s,               {RecordValue::Type::Integer,    "prog"s, 1}},
      {"prog.total_cpu_cores"s,               {RecordValue::Type::Integer,    "prog"s, 1}},
      {"prog.available_cpu_cores"s,           {RecordValue::Type::Integer,    "prog"s, 1}},
      {"prog.program_name"s,                  {RecordValue::Type::String,     "prog"s, 1}},
      {"prog.process_id"s,                    {RecordValue::Type::Integer,    "prog"s, 1}},
      {"prog.build_mode"s,                    {RecordValue::Type::String,     "prog"s, 1}},

      {"evt.record_type"s,                    {RecordValue::Type::String,     "evt"s, 1}},
      {"evt.event_category"s,                 {RecordValue::Type::String,     "evt"s, 1}},
      {"evt.event"s,                          {RecordValue::Type::String,     "evt"s, 1}},

      {"loc.file"s,                           {RecordValue::Type::String,     "loc"s, 1}},
      {"loc.line"s,                           {RecordValue::Type::Integer,    "loc"s, 1}},
      {"loc.function"s,                       {RecordValue::Type::String,     "loc"s, 1}},

      {"trc.function"s,                       {RecordValue::Type::String,     "trc"s, 1}},
      {"trc.parent_function"s,                {RecordValue::Type::String,     "trc"s, 1}},

      {"val.timestamp"s,                      {RecordValue::Type::Timestamp,  "val"s, 1}},
      {"val.thread_id"s,                      {RecordValue::Type::Integer,    "val"s, 1}},
      {"val.cpu_id"s,                         {RecordValue::Type::Integer,    "val"s, 1}},
      {"val.available_memory"s,               {RecordValue::Type::Integer,    "val"s, 1}},
      {"val.message"s,                        {RecordValue::Type::String,     "val"s, 1}},

      {"prof.workload"s,                      {RecordValue::Type::Real,       "prof.all"s, 1}},
      {"prof.duration"s,                      {RecordValue::Type::Real,       "prof.all"s, ns_to_sec}},

      {"prof.sw.cpu_clock"s,                  {RecordValue::Type::Real,       "prof.linux.sw"s, ns_to_sec}},
      {"prof.sw.task_clock"s,                 {RecordValue::Type::Real,       "prof.linux.sw"s, ns_to_sec}},
      {"prof.sw.page_faults"s,                {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.context_switches"s,           {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.cpu_migrations"s,             {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.page_faults_min"s,            {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.page_faults_maj"s,            {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.alignment_faults"s,           {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},
      {"prof.sw.emulation_faults"s,           {RecordValue::Type::Integer,    "prof.linux.sw"s, 1}},

      {"prof.hw.cpu_cycles"s,                 {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},
      {"prof.hw.instructions"s,               {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},

      {"prof.hw.stall_cycles_front"s,         {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},
      {"prof.hw.stall_cycles_back"s,          {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},

      {"prof.hw.branch_instructions"s,        {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},
      {"prof.hw.branch_misses"s,              {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},

      {"prof.hw.cache_references"s,           {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}},
      {"prof.hw.cache_misses"s,               {RecordValue::Type::Integer,    "prof.linux.hw"s, 1}}
  };

  return record_catalog;
}

// -----------------------------------------------------------------------------
/// used to validate the Record keys
bool is_valid_record_key(const std::string_view key) {
  if (key.starts_with("attr.")) {
    return true;
  }

  const RecordCatalog& record_catalog = get_record_catalog();
  return record_catalog.contains(std::string{key});
}

// -----------------------------------------------------------------------------
/// return a sorted vector of the keys
// it is probably faster to copy then sort the keys rather than insert while sorting
std::vector<std::string> get_sorted_record_keys() {
  const RecordCatalog& record_catalog = get_record_catalog();
  std::vector<std::string> keys;
  keys.reserve(record_catalog.size());

  for (const auto& [k, v] : record_catalog) {
    keys.emplace_back(k);
  }

  std::sort(std::begin(keys), std::end(keys));
  return keys;
}

// -----------------------------------------------------------------------------
/// return a sorted vector of the keys that are in the given categories
std::vector<std::string> get_keys_matching_categories(const std::set<std::string>& categories) {
  const std::vector<std::string> keys{get_sorted_record_keys()};
  std::vector<std::string> result_keys;
  for (const auto& key : keys) {
    if (categories.contains(key)) {
      result_keys.emplace_back(key);
    }
  }
  return result_keys;
}

// -----------------------------------------------------------------------------
/// get the RecordValue Type for the given record key name
RecordValue::Type get_record_type(std::string_view key) {
  const RecordCatalog& record_catalog = get_record_catalog();
  return get<0>(record_catalog.at(std::string(key)));
}

// -----------------------------------------------------------------------------
/// get the category for the given record key name
std::string get_record_category(std::string_view key) {
  const RecordCatalog& record_catalog = get_record_catalog();
  return get<1>(record_catalog.at(std::string(key)));
}

// -----------------------------------------------------------------------------
/// get the scale Type for the given record key name
double get_record_scale(std::string_view key) {
  const RecordCatalog& record_catalog = get_record_catalog();
  return get<2>(record_catalog.at(std::string(key)));
}

// -----------------------------------------------------------------------------
/// convert a source_location into a string
// this is typically merged with other record maps
Record source_location_to_record(const source_location& source_location)
{
  return Record{
      {"loc.file",     source_location.file_name()},
      {"loc.line",     source_location.line()},
      {"loc.function", source_location.function_name()}
  };
}

// -----------------------------------------------------------------------------
/// convert a Record to a JSON string
std::string record_to_json(const std::vector<std::string>& fields, std::shared_ptr<Record> record)
{
    std::stringstream buffer;
    bool first_field = true;
    buffer.put('{');

    for (const auto& field : fields) {
      if (!record->contains(field)) {
        continue;   // JSON skips missing fields from the record
      }
      const RecordValue value = record->at(field);

      if (first_field) {
        first_field = false;
      } else {
        buffer.put(',');
      }

      switch (value.get_type()) {
        case RecordValue::Type::Boolean: {
          buffer << format("\"{}\":{}", field, value.get_boolean());
          break;
        }

        case RecordValue::Type::Integer: {
          const double scale = get_record_scale(field);
          if (scale == 1) {
            buffer << format("\"{}\":{}", field, value.get_integer());
          } else {
            buffer << format("\"{}\":{}", field, static_cast<double>(value.get_integer())*scale);
          }
          break;
        }

        case RecordValue::Type::Real: {
          const double scale = get_record_scale(field);
          buffer << format("\"{}\":{}", field, value.get_real()*scale);
          break;
        }

        case RecordValue::Type::String: {
          buffer << format("\"{}\":\"{}\"", field, value.get_string());
          break;
        }

        case RecordValue::Type::Timestamp: {
          buffer << format("\"{}\":\"{}\"", field, format_timestamp(value.get_timestamp()));
          break;
        }

        default: assert(false);
      }
    }

    buffer << "}\n";
    return buffer.str();
}

// -----------------------------------------------------------------------------
// these are updated at dev::Function class
static inline thread_local std::string g_parent_function_name;
static inline thread_local std::string g_function_name;

// -----------------------------------------------------------------------------
static inline thread_local Record g_attributes;

// -----------------------------------------------------------------------------
/// User-defined global attributes
// the attribute names begin with "attr."
// we update a copy of the parent object, so it contains all active attributes
class Attributes
{
public:
    Attributes(const RecordInit& attribute_init)
    : _parent_attribute_map{g_attributes} {
        for (const auto& [k, v] : attribute_init) {
            g_attributes["attr." + k] = v;
        }
    }

    ~Attributes() {
        g_attributes = _parent_attribute_map;
    }

private:
    Record _parent_attribute_map;
};

// -----------------------------------------------------------------------------
/// read program-wide variables
// these values are constant per program run
Record create_program_record() {
  Record record{
      {"evt.record_type"s,          "program"sv},
      {"prog.run_id"s,              get_run_id()},
      {"prog.start_ts"s,            now()},
      {"prog.memory_page_size"s,    get_memory_page_size()},
      {"prog.physical_memory"s,     get_physical_memory()},
      {"prog.total_cpu_cores"s,     get_total_cpu_cores()},
      {"prog.available_cpu_cores"s, get_available_cpu_cores()},
      {"prog.program_name"s,        get_program_name()},
      {"prog.process_id"s,          get_process_id()},
      {"prog.build_mode"s,          get_build_mode_name()},
      {"prog.compiler"s,            get_compiler_name()},
      {"prog.platform"s,            get_platform_name()},
      {"prog.architecture"s,        get_architecture()},
      {"prog.real_username"s,       get_real_username()},
      {"prog.effective_username"s,  get_effective_username()}
  };
  return record;
}

// -----------------------------------------------------------------------------
/// create Record with common location fields for an event
Record create_location_record(const source_location& source_location)
{
  Record record{
      {"loc.file",                source_location.file_name()},
      {"loc.line",                source_location.line()},
      {"loc.function",            source_location.function_name()},

      {"trc.function",            g_function_name},
      {"trc.parent_function",     g_parent_function_name},

      {"val.timestamp",           now()},
      {"val.thread_id"s,          get_thread_id()},
      {"val.node_id"s,            get_node_id()},
      {"val.cpu_id"s,             get_cpu_id()},
      {"val.available_memory"s,   get_available_memory()}
  };

  return record;
}

// -----------------------------------------------------------------------------
/// create Record with fields for message events
Record create_message_record(const source_location& source_location,
                             std::string_view event_category,
                             std::string_view event,
                             std::string_view message)
{
  Record record{create_location_record(source_location)};

  record.insert({
      {"evt.record_type"s,        "message"sv},
      {"evt.event_category"s,     event_category},
      {"evt.event"s,              event},

      {"val.message"s,            message}
  });

  record.merge(g_attributes);
  return record;
}

// -----------------------------------------------------------------------------
/// create Record with common fields for an event
Record create_profile_record(const source_location& source_location, std::string_view record_type, double workload)
{
  Record record{create_location_record(source_location)};

  record.insert({
      {"evt.record_type"s,        record_type},
      {"evt.event_category"s,     "profile"sv},
      {"prof.workload"s,          workload}
  });

  record.merge(g_attributes);
  return record;
}

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
/// hash function for RecordValue
// defined in std namespace so it is used automatically
template<>
struct std::hash<giopler::RecordValue> {
  std::size_t operator()(const giopler::RecordValue& record_value) const {
    std::size_t seed = 0;
    giopler::hash_combine(seed,
      record_value._record_value_type,
      record_value._boolean_value,
      record_value._integer_value,
      record_value._real_value,
      record_value._string_value,
      record_value._timestamp_value);
    return seed;
  }
};

// -----------------------------------------------------------------------------
/// hash function for Record
// defined in std namespace so it is used automatically
template<>
struct std::hash<giopler::Record> {
  std::size_t operator()(const giopler::Record& record) const {
    // sort the entries to ensure consistent hashing
    std::vector<std::string> keys;
    keys.reserve(record.size());
    for (const auto& [key, value] : record) {
      keys.emplace_back(key);
    }
    std::sort(begin(keys), end(keys));

    std::size_t seed = 0;
    for (const auto& [key, value] : record) {
      giopler::hash_combine(seed, key, value);
    }
    return seed;
  }
};

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_RECORD_HPP
