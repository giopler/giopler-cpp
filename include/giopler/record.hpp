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
class RecordValue;

// -----------------------------------------------------------------------------
/// Data being sent to a sink for processing.
// Important: Record objects should NOT be modified after they are shared.
using Record          = std::unordered_map<std::string, RecordValue>;
using RecordInitList  = std::initializer_list<Record::value_type>;
using Array           = std::vector<RecordValue>;

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
  enum class Type {Empty, Boolean, Integer, Real, String, Timestamp, Record, Array};

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
        case Type::Record:      return _record_value    == record_value._record_value;
        case Type::Array:       return _array_value     == record_value._array_value;
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

  // ---------------------------------------------------------------------------
  RecordValue(std::shared_ptr<giopler::Record> record_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Record), _record_value(record_value) { }

  [[nodiscard]] std::shared_ptr<giopler::Record> get_record() const {
    assert(_record_value_type == Type::Record);
    return _record_value;
  }

  void set_record(const std::shared_ptr<giopler::Record> record_value) {
    assert(_record_value_type == Type::Record);
    _record_value = record_value;
  }

  // ---------------------------------------------------------------------------
  RecordValue(std::shared_ptr<giopler::Array> array_value)   // NOLINT(google-explicit-constructor)
  : _record_value_type(Type::Array), _array_value(array_value) { }

  [[nodiscard]] std::shared_ptr<giopler::Array> get_array() const {
    assert(_record_value_type == Type::Array);
    return _array_value;
  }

  void set_array(const std::shared_ptr<giopler::Array> array_value) {
    assert(_record_value_type == Type::Array);
    _array_value = array_value;
  }

  // ---------------------------------------------------------------------------
 private:
  friend struct std::hash<giopler::RecordValue>;
  Type _record_value_type;
  bool _boolean_value{};
  int64_t _integer_value{};
  double _real_value{};
  std::string _string_value{};
  Timestamp _timestamp_value{};
  std::shared_ptr<giopler::Record> _record_value;
  std::shared_ptr<giopler::Array>  _array_value;
};

// -----------------------------------------------------------------------------
/// catalog of all valid Record keys
// the values are already sorted
const std::vector<std::string>& get_record_catalog() {
  static const std::vector<std::string> record_catalog {
      "evt.event"s,
      "evt.event_category"s,
      "evt.event_id"s,
      "evt.record_type"s,
      "loc.file"s,
      "loc.function"s,
      "loc.line"s,
      "prof.duration"s,
      "prof.hw.branch_instructions"s,
      "prof.hw.branch_misses"s,
      "prof.hw.cache_misses"s
      "prof.hw.cache_references"s,
      "prof.hw.cpu_cycles"s,
      "prof.hw.instructions"s,
      "prof.hw.stall_cycles_back"s,
      "prof.hw.stall_cycles_front"s,
      "prof.sw.alignment_faults"s,
      "prof.sw.context_switches"s,
      "prof.sw.cpu_clock"s,
      "prof.sw.cpu_migrations"s,
      "prof.sw.emulation_faults"s,
      "prof.sw.page_faults"s,
      "prof.sw.page_faults_maj"s,
      "prof.sw.page_faults_min"s,
      "prof.sw.task_clock"s,
      "prof.workload"s,
      "prog.available_cpu_cores"s,
      "prog.build_mode"s,
      "prog.memory_page_size"s,
      "prog.physical_memory"s,
      "prog.process_id"s,
      "prog.program_name"s,
      "prog.run_id"s,
      "prog.start_ts"s,
      "prog.total_cpu_cores"s,
      "trc.function"s,
      "trc.parent_function"s,
      "val.available_memory"s,
      "val.cpu_id"s,
      "val.message"s,
      "val.thread_id"s,
      "val.timestamp"s
  };

  return record_catalog;
}

// -----------------------------------------------------------------------------
/// return a sorted vector of the keys
std::vector<std::string> get_sorted_record_keys() {
  return get_record_catalog();
}

// -----------------------------------------------------------------------------
/// User-defined global attributes
// the attribute names begin with "attr."
// we update a copy of the parent object, so it contains all active attributes
// this way we avoid a potential long recursive loop on every data request
class Attributes
{
public:
  Attributes(const RecordInitList& attribute_init)
  : _parent_attributes{_attributes},
    _data{_attributes ? _attributes->_data : std::make_shared<Record>()}
  {
    _attributes = this;

    for (const auto& [key, value] : attribute_init) {
      const std::string attr_key = key.starts_with("attr."sv) ? key : ("attr." + key);
      (*_data)[attr_key] = to_json_string(value);
    }
  }

  ~Attributes() {
      _attributes = _parent_attributes;
  }

  static std::shared_ptr<giopler::Record> get_attributes() {
    return _attributes ? _attributes->_data : std::make_shared<Record>();
  }

private:
  static inline thread_local Attributes* _attributes = nullptr;
  Attributes* _parent_attributes;
  std::shared_ptr<giopler::Record> _data;

  /// force the attribute value to be a JSON string
  static std::string to_json_string(const RecordValue& value) {
    switch (value.get_type()) {
      case RecordValue::Type::Boolean: {
        return format("\"{}\"", value.get_boolean());
      }

      case RecordValue::Type::Integer: {
        return format("\"{}\"", value.get_integer());
      }

      case RecordValue::Type::Real: {
        return format("\"{}\"", value.get_real());
      }

      case RecordValue::Type::String: {
        return format("\"{}\"", value.get_string());
      }

      case RecordValue::Type::Timestamp: {
        return format("\"{}\"", format_timestamp(value.get_timestamp()));
      }

      case RecordValue::Type::Record: {
        assert(false);
      }

      case RecordValue::Type::Array: {
        assert(false);
      }

      case RecordValue::Type::Empty: {
        return "\"null\"";
      }
    }
  }
};

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
void record_value_to_json(const RecordValue& value, std::stringstream& buffer)
{
    switch (value.get_type()) {
      case RecordValue::Type::Boolean: {
        buffer << format("{}", value.get_boolean());
        break;
      }

      case RecordValue::Type::Integer: {
        buffer << format("{}", value.get_integer());
        break;
      }

      case RecordValue::Type::Real: {
        buffer << format("{}", value.get_real());
        break;
      }

      case RecordValue::Type::String: {
        buffer << format("\"{}\"", value.get_string());
        break;
      }

      case RecordValue::Type::Timestamp: {
        buffer << format("\"{}\"", format_timestamp(value.get_timestamp()));
        break;
      }

      case RecordValue::Type::Record: {
        buffer.put('{');
        bool first_field = true;

        for (const auto& [rec_field, rec_value] : *(value.get_record())) {
          if (first_field) {
            first_field = false;
          } else {
            buffer.put(',');
          }

          buffer << format("\"{}\":", rec_field);
          record_value_to_json(rec_value, buffer);
        }

        buffer << "}\n";
        break;
      }

      case RecordValue::Type::Array: {
        buffer.put('[');
        bool first_field = true;

        for (const auto& array_value : *(value.get_array())) {
          if (first_field) {
            first_field = false;
          } else {
            buffer.put(',');
          }

          record_value_to_json(array_value, buffer);
        }

        buffer << "]\n";
        break;
      }

      case RecordValue::Type::Empty: {
        buffer << "null";
        break;
      }
    }
}

// -----------------------------------------------------------------------------
/// convert a Record to a JSON string
std::string record_to_json(std::shared_ptr<Record> record)
{
    std::stringstream buffer;
    record_value_to_json(RecordValue(record), buffer);
    return buffer.str();
}

// -----------------------------------------------------------------------------
/// convert a Record to a CSV string
std::string record_to_csv(const std::vector<std::string>& fields, std::shared_ptr<Record> record,
                          std::string_view _separator = ","sv, std::string_view _string_quote = "\""sv)
{
    std::stringstream buffer;
    bool first_field = true;
    for (const auto& field : fields) {
      // do not print anything for missing field values
      const RecordValue value = record->contains(field) ? record->at(field) : RecordValue();

      if (first_field) {
        first_field = false;
      } else {
        buffer << _separator;
      }

      switch (value.get_type()) {
        case RecordValue::Type::Boolean: {
          buffer << format("{}", value.get_boolean());
          break;
        }

        case RecordValue::Type::Integer: {
          buffer << format("{}", value.get_integer());
          break;
        }

        case RecordValue::Type::Real: {
          buffer << format("{}", value.get_real());
          break;
        }

        case RecordValue::Type::String: {
          buffer << format("{0}{1}{0}", _string_quote, value.get_string());
          break;
        }

        case RecordValue::Type::Timestamp: {
          buffer << format("{0}{1}{0}", _string_quote, format_timestamp(value.get_timestamp()));
          break;
        }

        case RecordValue::Type::Empty: {
          break;
        }

        default: assert(false);
      }
    }

    buffer.put('\n');
    return buffer.str();
}

// -----------------------------------------------------------------------------
// these are updated at dev::Function class
static inline thread_local std::string g_parent_function_name;
static inline thread_local std::string g_function_name;

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
      {"prog.host_name"s,           get_host_name()},
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
                             std::string_view event_id,
                             std::string_view event_category,
                             std::string_view event,
                             const double workload,
                             std::string_view message)
{
  Record record{create_location_record(source_location)};

  record.insert({
      {"evt.record_type"s,        "message"sv},
      {"evt.event_id"s,           event_id},
      {"evt.event_category"s,     event_category},
      {"evt.event"s,              event},

      {"val.attributes"s,         Attributes::get_attributes()},
      {"val.workload"s,           workload},
      {"val.message"s,            message}
  });

  return record;
}

// -----------------------------------------------------------------------------
/// create Record with common fields for an event
Record create_profile_record(const source_location& source_location,
                             std::string_view record_type,
                             std::string_view event_id,
                             std::string_view event,
                             const double workload,
                             std::shared_ptr<Record> total)
{
  Record record{create_location_record(source_location)};

  record.insert({
      {"evt.record_type"s,        record_type},
      {"evt.event_id"s,           event_id},
      {"evt.event_category"s,     "profile"sv},
      {"evt.event"s,              event},

      {"val.attributes"s,         Attributes::get_attributes()},
      {"val.workload"s,           workload},
      {"prof.total"s,             total}
  });

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
