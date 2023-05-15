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
#include <forward_list>
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
class SinkManager final {
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
      _workers.emplace_front(std::async(std::launch::async, [&sink, record]{return sink->write_record(record);}));
    }
  }

  /// write uncommitted changes to the underlying output sequences
  void flush() {
    {
      const std::lock_guard<std::mutex> lock{_mutex};
      if (!_workers.empty()) {
        std::cout << "Sending remaining events to Giopler..." << std::endl;
      }
      wait_workers();
    }
    for (auto&& sink : _sinks) {
      sink->flush();
    }
  }

 private:
  /// Sink objects are not copied but are called from multiple threads, one for each worker.
  std::vector<std::unique_ptr<Sink>> _sinks;
  std::once_flag _create_sinks_once_flag;
  std::mutex _mutex;
  std::forward_list<std::future<bool>> _workers;

  /// Create default sinks if write attempted and no sinks defined already
  static void create_sinks();

  /// Check and remove workers that have already finished executing.
  void check_workers() {
    _workers.remove_if([](std::future<bool>& fut)
        { return fut.wait_for(0s) == std::future_status::ready; });
  }

  /// Wait for the workers to finish and then delete them.
  void wait_workers() {
      _workers.remove_if([](std::future<bool> &fut) {
        fut.wait();
        return true;
      });
  }
};

// -----------------------------------------------------------------------------
static inline SinkManager g_sink_manager{};

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
  if (std::getenv("GIOPLER_TOKEN")) {
    Rest::add_sink();
  } else {
    throw std::runtime_error{"GIOPLER_TOKEN not defined"};
  }
}

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_SINK_HPP
