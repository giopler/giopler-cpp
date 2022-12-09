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
#include <memory>
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
/// general category for the event
enum class EventCategory {Contract, Trace, Log, Profile, Test, Bench};

// -----------------------------------------------------------------------------
/// convert the event category enum value into a string
constexpr std::string_view get_event_category_name(const EventCategory event_category) {
  switch (event_category) {
    case EventCategory::Contract:   return "contract"sv;
    case EventCategory::Trace:      return "trace"sv;
    case EventCategory::Log:        return "log"sv;
    case EventCategory::Profile:    return "profile"sv;
    case EventCategory::Test:       return "test"sv;
    case EventCategory::Bench:      return "bench"sv;
  }
  return "UnknownEventCategory"sv;
}

// -----------------------------------------------------------------------------
/// uniquely identifies the event
enum class Event {
                  // Contract
                  Argument,
                  Expect,
                  Confirm,
                  InvariantBegin,
                  InvariantEnd,
                  Ensure,
                  Certify,

                  // Trace
                  Line,
                  Branch,

                  // Log
                  Warning,
                  Error,
                  Message,

                  // Profile
                  ProgramBegin,
                  ProgramEnd,
                  ThreadBegin,
                  ThreadEnd,
                  FunctionBegin,
                  FunctionEnd,
                  ObjectBegin,
                  ObjectEnd
};

// -----------------------------------------------------------------------------
/// convert the event category enum value into a string
constexpr std::string_view get_event_name(const Event event) {
  switch (event) {
    case Event::Argument:       return "argument"sv;
    case Event::Expect:         return "expect"sv;
    case Event::Confirm:        return "confirm"sv;
    case Event::InvariantBegin: return "invariant_begin"sv;
    case Event::InvariantEnd:   return "invariant_end"sv;
    case Event::Ensure:         return "ensure"sv;
    case Event::Certify:        return "certify"sv;

    case Event::Line:           return "line"sv;
    case Event::Branch:         return "branch"sv;

    case Event::Warning:        return "warning"sv;
    case Event::Error:          return "error"sv;
    case Event::Message:        return "message"sv;

    case Event::ProgramBegin:   return "program_begin"sv;
    case Event::ProgramEnd:     return "program_end"sv;
    case Event::ThreadBegin:    return "thread_begin"sv;
    case Event::ThreadEnd:      return "thread_end"sv;
    case Event::FunctionBegin:  return "function_begin"sv;
    case Event::FunctionEnd:    return "function_end"sv;
    case Event::ObjectBegin:    return "object_begin"sv;
    case Event::ObjectEnd:      return "object_end"sv;
  }
  return "UnknownEvent"sv;
}

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
/// User-defined attributes
// these are automatically included with events sent
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
      (*_data)[key] = to_json_string(value);
    }
  }

  ~Attributes() {
      _attributes = _parent_attributes;
  }

  static std::shared_ptr<giopler::Record> get_attributes_record() {
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

      case RecordValue::Type::Timestamp: {   // not sure if this is needed
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
/// returns the unique UUID for the program run
UUID get_run_id() {
  static const UUID run_id{UUID{}};
  return run_id;
}

// -----------------------------------------------------------------------------
/// read program-wide variables
// these values are constant per program run
std::shared_ptr<Record> get_program_record() {
  return std::make_shared<Record>(Record{
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
  });
}

// -----------------------------------------------------------------------------
std::shared_ptr<Record> get_event_record(const source_location& source_location,
                                         EventCategory event_category,
                                         Event event,
                                         const UUID& event_object_id = UUID::get_nil(),
                                         const double workload = 0,
                                         const std::string_view message = ""sv)
{
  return std::make_shared<Record>(Record{
      {"prog.run_id"s,            get_run_id().get_string()},

      {"evt.event_category"s,     get_event_category_name(event_category)},
      {"evt.event"s,              get_event_name(event)},
      {"evt.event_object_id"s,    event_object_id.get_string()},

      {"evt.file"s,               source_location.file_name()},
      {"evt.line"s,               source_location.line()},
      {"evt.function"s,           source_location.function_name()},

      {"evt.timestamp"s,          now()},
      {"evt.thread_id"s,          get_thread_id()},
      {"evt.node_id"s,            get_node_id()},
      {"evt.cpu_id"s,             get_cpu_id()},
      {"evt.available_memory"s,   get_available_memory()},

      {"evt.workload"s,           workload},
      {"evt.message"s,            message}
  });
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
      record_value._timestamp_value,
      record_value._record_value,
      record_value._array_value);
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
