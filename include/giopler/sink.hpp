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
#ifndef GIOPLER_SINK_HPP
#define GIOPLER_SINK_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <deque>
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
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
using namespace std::chrono_literals;

#include "giopler/config.hpp"
#include "giopler/utility.hpp"
#include "giopler/record.hpp"

// -----------------------------------------------------------------------------
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
#if defined(GIOPLER_PLATFORM_LINUX)
#include "giopler/linux/rest_sink.hpp"
#endif

// -----------------------------------------------------------------------------
namespace giopler::sink {

// -----------------------------------------------------------------------------
/// Sink management class. Thread safe.
class SinkManager final {
 public:
  explicit SinkManager()
  {
    _quiet = std::getenv("GIOPLER_QUIET");   // convert pointer to boolean

    if (!std::getenv("GIOPLER_TOKEN")) {
      throw std::runtime_error{"GIOPLER_TOKEN not defined"};
    }

    const int openssl_status = OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, nullptr);
    ERR_print_errors_fp(stderr);
    assert(openssl_status == 1);

    _processes.reserve(_process_count);
    for (int process = 0; process < _process_count; ++process) {
      _processes.emplace_back(_process_function);
    }
  }

  /// Waits for all sinks to finish processing data before exiting.
  ~SinkManager() {
    {
      const std::lock_guard<std::mutex> lock{_deque_mutex};
      const std::size_t records_count = _deque_records.size();
      if (records_count && !_quiet)
        std::cout << gformat("Giopler: sending remaining {} event{} to Giopler system\n", records_count, (records_count > 1) ? "s" : "");
    }

    for (auto&& process : _processes) {
      process.request_stop();
    }
    _cond_var.notify_all();
    for (auto&& process : _processes) {
      process.join();
    }
    assert(_deque_records.empty());   // all process threads should have exited by now
  }

  /// Write the record to the sink.
  // Sink objects run in their own thread.
  // using default of std::launch::async|std::launch::deferred
  // Gcc appears to try async first, which is what we want.
  static void write_record(std::shared_ptr<Record> record) {
    {
      const std::lock_guard<std::mutex> lock{_deque_mutex};
      _deque_records.emplace_back(record);   // shared_ptr is thread safe
    }

    _cond_var.notify_one();   // will wake up one of the sleeping threads, if any
  }

  /// write uncommitted changes to the underlying output sequences
  // we wait no more than thirty seconds for the events to be sent
  static void flush() {
    std::size_t events_count;
    int waits_count = 0;

    do {
      {
        const std::lock_guard<std::mutex> lock{_deque_mutex};
        events_count = _deque_records.size();
      }
      if (events_count) {
        std::this_thread::sleep_for(1s);    // one second
        waits_count++;
      }
    } while (events_count && waits_count < 30);   // thirty seconds
  }

 private:
  // Warning: Please resist the temptation to adjust these values.
  // Increasing _process_count leads to higher lock contention at the servers.
  // This leads to lower overall throughput, not higher.
  // Keeping max_records_size small is better for user feedback.
  // Increasing it will not result in significantly higher throughput.
  static constexpr int _process_count = 4;                                // processes to send events to the server
  static constexpr std::size_t max_records_size = 4*1024*1024;            // JSON bytes before gzip compression

  static inline std::deque<std::shared_ptr<Record>> _deque_records;
  static inline std::mutex _deque_mutex;
  static inline std::mutex _cond_var_mutex;
  static inline std::condition_variable _cond_var;
  static inline bool _quiet = false;
  std::vector<std::jthread> _processes;

  // will take on average half a second to exit once requested
  constexpr static auto _process_function = [](std::stop_token stop_token) -> void {
    Rest sink;
    std::size_t records_left = 0;
    bool is_stop_requested = false;

    do {   // loop waiting for data to send or for the program to signal it is done
      {
        std::unique_lock<std::mutex> lock{_cond_var_mutex};
        _cond_var.wait_for(lock, 1s);   // wait for new records to process or one second
      }

      std::string records = "[";
      records.reserve(max_records_size + 2048);   // 2048=a record should be smaller than this
      int records_count = 0;
      bool first = true;

      {
        const std::lock_guard<std::mutex> lock{_deque_mutex};
        while (!_deque_records.empty()) {
          if (first) {
            first = false;
          } else {
            records.append(",");
          }
          records.append(record_to_json(_deque_records.front()));
          _deque_records.pop_front();
          records_count++;
          if (records.length() > max_records_size) {
            break;
          }
        }
        records.append("]");
      }
      if (records_count) {
        const TimestampSteady start_time = now_steady();
        sink.write_records(records);
        const double time_secs = timestamp_diff(start_time, now_steady());
        if (!_quiet)
          std::cout << gformat("Giopler: sent {} event{} to Giopler system ({:.2f} events/second)\n",
                               records_count, (records_count > 1) ? "s" : "", records_count/time_secs);
      }
      {
        is_stop_requested = stop_token.stop_requested();   // need to read this before getting records count
        const std::lock_guard<std::mutex> lock{_deque_mutex};
        records_left = _deque_records.size();
      }
    } while (records_left || !is_stop_requested);   // hang around in case we get more work to do in future
  };
};

// -----------------------------------------------------------------------------
static inline SinkManager g_sink_manager{};

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_SINK_HPP
