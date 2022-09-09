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
#ifndef GIOPLER_SINK_HPP
#define GIOPLER_SINK_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
using namespace std::chrono_literals;

#include "giopler/config.hpp"
#include "giopler/utility.hpp"
#include "giopler/record.hpp"

// -----------------------------------------------------------------------------
namespace giopler::sink {

// the C++ standard library is not guaranteed to be thread safe
// file i/o operations are thread-safe on Windows and on POSIX systems
// the POSIX standard requires that C stdio FILE* operations are atomic
// https://docs.microsoft.com/en-us/cpp/standard-library/thread-safety-in-the-cpp-standard-library
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_concurrency.html
// https://en.cppreference.com/w/cpp/io/ios_base/sync_with_stdio
// Regarding cerr, cout, clog, etc:
//   Unless std::ios_base::sync_with_stdio(false) has been issued,
//   it is safe to concurrently access these objects from multiple threads for both formatted and unformatted output.
//   Due to the overhead necessitated by thread-safety, no other stream objects are thread-safe by default.

// -----------------------------------------------------------------------------
struct Sink {
  virtual ~Sink() = default;
  virtual bool write_record(std::shared_ptr<Record> record) = 0;
  virtual void flush() = 0;
};

// -----------------------------------------------------------------------------
/// Sink management class. Thread safe.
class SinkManager {
 public:
  SinkManager() : _sinks{}, _workers{} { }

  /// Waits for all sinks to finish processing data before exiting.
  ~SinkManager() {
    flush();
  }

  /// add another sink to the chain
  void add_sink(std::unique_ptr<Sink> sink) {
    const std::lock_guard<std::mutex> lock{_mutex};
    _sinks.emplace_back(std::move(sink));
  }

  /// delete all the sinks already added
  void delete_sinks() {
    const std::lock_guard<std::mutex> lock{_mutex};
    _sinks.clear();
  }

  /// Write the record to the sink.
  // Sink objects run in their own thread.
  void write_record(std::shared_ptr<Record> record) {
    std::call_once(_create_sinks_once_flag, create_sinks);
    const std::lock_guard<std::mutex> lock{_mutex};
    check_workers();   // check before adding to avoid checking newly added workers
    for (auto&& sink : _sinks) {
      _workers.emplace_back(std::async(std::launch::async, [&sink, record]{return sink->write_record(record);}));
    }
  }

  /// write uncommitted changes to the underlying output sequences
  void flush() {
    {
      const std::lock_guard<std::mutex> lock{_mutex};
      wait_workers();
    }
    for (auto&& sink : _sinks) {
      sink->flush();
    }
  }

 private:
  /// Sink objects are not copied but are called from multiple threads, one for each worker.
  std::vector<std::unique_ptr<Sink>> _sinks;
  std::list<std::future<bool>> _workers;
  std::mutex _mutex;
  std::once_flag _create_sinks_once_flag;

  /// Create default sinks if write attempted and no sinks defined already
  static void create_sinks();

  /// Check and remove workers that have already finished executing.
  void check_workers() {
    _workers.remove_if([](std::future<bool>& fut)
      { return fut.wait_for(0s) == std::future_status::ready; });
  }

  /// Wait for the workers to finish and then delete them.
  void wait_workers() {
    _workers.remove_if([](std::future<bool>& fut)
      { fut.wait(); return true; });
  }
};

// -----------------------------------------------------------------------------
static inline SinkManager g_sink_manager{};

// -----------------------------------------------------------------------------
/// log file destination using JSON format
// https://jsonlines.org/
// https://www.json.org/
class Json : public Sink {
 public:
  Json(std::vector<std::string> fields, std::string_view filepath)
  :
    _output_stream{get_output_filepath(filepath, "json"sv)},
    _osync_stream{std::osyncstream(*_output_stream)},
    _fields(std::move(fields))
  { }

  ~Json() override = default;

  /// add a new JSON format data record sink
  // empty fields list = add all fields in sorted order
  // Directory patterns:
  //   <temp>, <current>, <home>   - optionally follow these with other directories
  //   <cout>, <clog>, <cerr>      - these specify the entire path
  static void add_sink(std::vector<std::string> fields = {}, std::string_view path = "<current>"sv) {
    if (fields.empty()) {
      fields = get_sorted_record_keys();
    }
    g_sink_manager.add_sink(std::make_unique<Json>(std::move(fields),path));
  }

 protected:
  bool write_record(std::shared_ptr<Record> record) override {
    if (is_record_filtered(*record)) {
      return false;
    }

    _osync_stream << record_to_json(_fields, record);
    return true;   // record was not filtered and it was written out
  }

  void flush() override {
    _osync_stream.emit();
    _output_stream->flush();
  }

 private:
  std::unique_ptr<std::ostream> _output_stream;
  std::osyncstream _osync_stream;
  std::vector<std::string> _fields;

  /// Skip writing this record due to filter conditions for sink?
  bool is_record_filtered(const Record& record) {
    return false;
  }
};

// -----------------------------------------------------------------------------
/// log file destination using CSV format
class Csv : public Sink {
 public:
  explicit Csv(std::vector<std::string> fields, std::string_view filepath = "<current>"sv,
      const bool header = true, std::string_view separator = ","sv, std::string_view string_quote = "\""sv)
  :
    _output_stream{get_output_filepath(filepath, "txt")},
    _osync_stream{std::osyncstream(*_output_stream)},
    _fields(std::move(fields)), _header(header), _separator(separator), _string_quote(string_quote)
  {
    if (_header)   add_header();
  }

  ~Csv() override = default;

  /// add a new CSV format data record sink
  // empty fields list = add all fields in sorted order
  // Directory patterns:
  //   <temp>, <current>, <home>   - optionally follow these with other directories
  //   <cout>, <clog>, <cerr>      - these specify the entire path
  static void add_sink(std::vector<std::string> fields = {}, std::string_view filepath = "<current>"sv,
    const bool header = true, std::string_view separator = ","sv, std::string_view string_quote = "\""sv)
  {
    if (fields.empty()) {
      fields = get_sorted_record_keys();
    }
    g_sink_manager.add_sink(std::make_unique<Csv>(std::move(fields), filepath, header, separator, string_quote));
  }

 protected:
  /// Add a first line header with the field names, if the option is enabled.
  // Guaranteed to happen before sink is enabled, so no need to buffer the output.
  void add_header() {
    bool first_field = true;
    for (const auto& field : _fields) {
      if (first_field) {
        first_field = false;
      } else {
        _osync_stream << _separator;
      }

      _osync_stream << field;
    }
    _osync_stream.put('\n');
  }

  bool write_record(std::shared_ptr<Record> record) override {
    if (is_record_filtered(*record)) {
      return false;
    }

    std::stringstream buffer;
    bool first_field = true;
    for (const auto& field : _fields) {
      // CSV has to always include the requested field values, even if missing from the record
      const RecordValue value = record->contains(field) ? record->at(field) : RecordValue(get_record_type(field));

      if (first_field) {
        first_field = false;
      } else {
        buffer << _separator;
      }

      switch (value.get_type()) {
        case RecordValue::Type::Bool: {
          buffer << format("{}", value.get_bool());
          break;
        }

        case RecordValue::Type::Integer: {
          const double scale = get_record_scale(field);
          if (scale == 1) {
          buffer << format("{}", value.get_integer());
          } else {
            buffer << format("{}", static_cast<double>(value.get_integer())*scale);
          }
          break;
        }

        case RecordValue::Type::Real: {
          const double scale = get_record_scale(field);
          buffer << format("{}", value.get_real()*scale);
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

        default: assert(false);
      }
    }

    buffer.put('\n');
    _osync_stream << buffer.view();
    return true;
  }

  void flush() override {
    _osync_stream.emit();
    _output_stream->flush();
  }

 private:
  std::unique_ptr<std::ostream> _output_stream;
  std::osyncstream _osync_stream;
  std::vector<std::string> _fields;
  bool _header;
  std::string_view _separator;
  std::string_view _string_quote;

  bool is_record_filtered(const Record& record) {
    return false;
  }
};

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#if defined(GIOPLER_PLATFORM_LINUX)
#include "giopler/linux/rest_sink.hpp"
#endif

// -----------------------------------------------------------------------------
namespace giopler::sink {

// -----------------------------------------------------------------------------
/// Create default sinks if write attempted and no sinks defined already
// Defined here, so we can refer to the sink classes.
void SinkManager::create_sinks() {
  if (g_sink_manager._sinks.empty()) {
    Csv::add_sink({"val.timestamp", "cat.category", "cat.event", "val.message"},
                  "<clog>", false, " ", "");

    if (std::getenv("GIOPLER_TOKEN")) {
      Rest::add_sink(std::getenv("GIOPLER_TOKEN"));
    } else {
      Json::add_sink();   // adds all record keys in sorted order
    }
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_SINK_HPP
