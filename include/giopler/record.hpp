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
    case EventCategory::Contract:   return "Contract"sv;
    case EventCategory::Trace:      return "Trace"sv;
    case EventCategory::Log:        return "Log"sv;
    case EventCategory::Profile:    return "Profile"sv;
    case EventCategory::Test:       return "Test"sv;
    case EventCategory::Bench:      return "Bench"sv;
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
    case Event::Argument:       return "Argument"sv;
    case Event::Expect:         return "Expect"sv;
    case Event::Confirm:        return "Confirm"sv;
    case Event::InvariantBegin: return "InvariantBegin"sv;
    case Event::InvariantEnd:   return "InvariantEnd"sv;
    case Event::Ensure:         return "Ensure"sv;
    case Event::Certify:        return "Certify"sv;

    case Event::Line:           return "Line"sv;
    case Event::Branch:         return "Branch"sv;

    case Event::Warning:        return "Warning"sv;
    case Event::Error:          return "Error"sv;
    case Event::Message:        return "Message"sv;

    case Event::ProgramBegin:   return "ProgramBegin"sv;
    case Event::ProgramEnd:     return "ProgramEnd"sv;
    case Event::ThreadBegin:    return "ThreadBegin"sv;
    case Event::ThreadEnd:      return "ThreadEnd"sv;
    case Event::FunctionBegin:  return "FunctionBegin"sv;
    case Event::FunctionEnd:    return "FunctionEnd"sv;
    case Event::ObjectBegin:    return "ObjectBegin"sv;
    case Event::ObjectEnd:      return "ObjectEnd"sv;
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
void record_value_to_json(const RecordValue& value, std::stringstream& buffer)
{
    switch (value.get_type()) {
      case RecordValue::Type::Boolean: {
        buffer << gformat("{}", value.get_boolean());
        break;
      }

      case RecordValue::Type::Integer: {
        buffer << gformat("{}", value.get_integer());
        break;
      }

      case RecordValue::Type::Real: {
        buffer << gformat("{}", value.get_real());
        break;
      }

      case RecordValue::Type::String: {
        buffer << gformat("\"{}\"", value.get_string());
        break;
      }

      case RecordValue::Type::Timestamp: {
        buffer << gformat("\"{}\"", format_timestamp(value.get_timestamp()));
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

          buffer << gformat("\"{}\":", rec_field);
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
/// convert the record value to a string
// does not support nested Array or Record values
std::string record_value_to_string(const RecordValue& value) {
  switch (value.get_type()) {
    case RecordValue::Type::Boolean: {
      return gformat("\"{}\"", value.get_boolean());
    }

    case RecordValue::Type::Integer: {
      return gformat("\"{}\"", value.get_integer());
    }

    case RecordValue::Type::Real: {
      return gformat("\"{}\"", value.get_real());
    }

    case RecordValue::Type::String: {
      return gformat("\"{}\"", value.get_string());
    }

    case RecordValue::Type::Timestamp: {   // not sure if this is needed
      return gformat("\"{}\"", format_timestamp(value.get_timestamp()));
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

    default: {
      assert(false);
      return ""s;
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
/// returns a value that increments once per call per thread
// useful as a fine-grained sequencing value for events within a thread
// timestamp values do not always have enough accuracy
int64_t get_thread_sequence() {
  static thread_local int64_t sequence = 0;
  return sequence++;
}

// -----------------------------------------------------------------------------
}   // namespace giopler


// -----------------------------------------------------------------------------
namespace giopler::dev {

// -----------------------------------------------------------------------------
/// set a thread-global class value
// use with a hierarchical dot-separated list of identifiers
// examples: "init.db", "disk.write.widget"
// intended to be used similarly to the HTML 'class' attribute
class Class final
{
 public:
  explicit Class(std::string_view class_value)
  : _parent_class{_current_class},
    _class_value{class_value}
  {
    _current_class = this;
  }

  ~Class() {
    _current_class = _parent_class;
  }

  static std::string get_class() {
    return _current_class ? _current_class->_class_value : "";
  }

 private:
  static inline thread_local Class* _current_class = nullptr;
  Class* _parent_class;
  std::string _class_value;
};

// -----------------------------------------------------------------------------
/// set a thread-global id value
// assign a unique value to it
// examples: "cust.12345", "frame.3232"
// intended to be used similarly to the HTML 'id' attribute
class Id final
{
 public:
  explicit Id(std::string_view id_value)
  : _parent_id{_current_id},
    _id_value{id_value}
  {
    _current_id = this;
  }

  ~Id() {
    _current_id = _parent_id;
  }

  static std::string get_id() {
    return _current_id ? _current_id->_id_value : "";
  }

 private:
  static inline thread_local Id* _current_id = nullptr;
  Id* _parent_id;
  std::string _id_value;
};

// -----------------------------------------------------------------------------
/// User-defined attributes
// these are automatically included with events sent
// we update a copy of the parent object, so it contains all active attributes
// this way we avoid a potential long recursive loop on every data request
// we convert all the values to JSON strings before storing them
// it is safe to share the _data pointer because it is not modified after created
class Attributes final
{
public:
  explicit Attributes(const RecordInitList& attribute_init)
  : _parent_attributes{_attributes},
    _data{_attributes ? _attributes->_data : std::make_shared<Record>()}
  {
    _attributes = this;

    for (const auto& [key, value] : attribute_init) {
      (*_data)[key] = record_value_to_string(value);
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
};

// -----------------------------------------------------------------------------
}   // namespace giopler::dev

// -----------------------------------------------------------------------------
namespace giopler {

// -----------------------------------------------------------------------------
/// read program-wide variables
// these values are constant per program run
std::shared_ptr<Record> get_program_record() {
  return std::make_shared<Record>(Record{
      {"start"s,            start_time},
      {"pgm"s,              get_program_name()},
      {"build"s,            get_build_mode_name()},

      {"compiler"s,         get_compiler_name()},
      {"platform"s,         get_platform_name()},
      {"arch"s,             get_architecture()},
      {"host"s,             get_host_name()},
      {"real_user"s,        get_real_username()},
      {"eff_user"s,         get_effective_username()},

      {"mem_page"s,         get_memory_page_size()},
      {"phys_mem"s,         get_physical_memory()},
      {"conf_cpu"s,         get_conf_cpu_cores()},
      {"avail_cpu"s,        get_available_cpu_cores()},
      {"proc_id"s,          get_process_id()}
  });
}

// -----------------------------------------------------------------------------
std::shared_ptr<Record> get_event_record(const source_location& source_location,
                                         const EventCategory event_category,
                                         const Event event,
                                         const UUID& event_id = UUID(),
                                         const UUID& event_other_id = UUID::get_nil(),
                                         const double workload = 0,
                                         const bool is_leaf = false,
                                         const std::string_view message = ""sv)
{
  return std::make_shared<Record>(Record{
      {"run_id"s,             get_run_id().get_string()},
      {"event_id"s,           event_id.get_string()},

      {"event_cat"s,          get_event_category_name(event_category)},
      {"event"s,              get_event_name(event)},
      {"other_id"s,           event_other_id.get_string()},

      {"time_diff"s,          get_time_delta()},
      {"thrd_seq"s,           get_thread_sequence()},

      {"file"s,               source_location.file_name()},
      {"line"s,               source_location.line()},
      {"func"s,               source_location.function_name()},
      {"msg"s,                message},

      {"thrd_id"s,            get_thread_id()},
      {"node_id"s,            get_node_id()},
      {"cpu_id"s,             get_cpu_id()},
      {"avail_mem"s,          get_available_memory()},

      {"clss"s,               giopler::dev::Class::get_class()},
      {"id"s,                 giopler::dev::Id::get_id()},
      {"wrkld"s,              workload},
      {"is_leaf"s,            is_leaf}
      //{"attributes"s,         giopler::dev::Attributes::get_attributes_record()}
  });
}

// -----------------------------------------------------------------------------
}   // namespace giopler

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
