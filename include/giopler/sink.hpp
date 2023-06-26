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
  explicit SinkManager() {
    if (!std::getenv("GIOPLER_TOKEN")) {
      throw std::runtime_error{"GIOPLER_TOKEN not defined"};
    }
  }

  /// Waits for all sinks to finish processing data before exiting.
  ~SinkManager() {
    flush();
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

    _cond_var.notify_one();   // only one listener anyway
  }

  /// write uncommitted changes to the underlying output sequences
  void flush() {
    _process_records.request_stop();
    _process_records.join();
  }

 private:
  static inline std::deque<std::shared_ptr<Record>> _deque_records;
  static inline std::condition_variable _cond_var;
  static inline std::mutex _cond_var_mutex;
  static inline std::mutex _deque_mutex;

  // will take on average half a second to exit once requested
  std::jthread _process_records{[](std::stop_token stop_token) -> void {
    std::this_thread::sleep_for(2000ms);
    Rest sink;
    std::size_t records_sent = 0;
    std::size_t records_left = 0;

    do {   // loop waiting for data to send or for the program to signal it is done
      {
        std::unique_lock<std::mutex> lock{_cond_var_mutex};
        _cond_var.wait_for(lock, 1s);   // wait for new records to process or one second
      }

      if (stop_token.stop_requested()) {
        const std::lock_guard<std::mutex> lock{_deque_mutex};
        records_left = _deque_records.size();
        if (records_left) {
          std::cout << "Sending remaining " << records_left << " events to the Giopler system..." << std::endl;
        }
      }

      do {   // loop over records waiting to be sent
        {
          const std::lock_guard<std::mutex> lock{_deque_mutex};
          if (!_deque_records.empty()) {
            sink.write_record(_deque_records.front());
            _deque_records.pop_front();
            records_sent++;
          }
          records_left = _deque_records.size();
        }

        if (stop_token.stop_requested()) {
          if ((records_left % 100) == 0) {
            std::cout << "Giopler events left to send: " << records_left << std::endl;
          }
        } else {
          std::this_thread::yield();   // yield to app if it is still running
        }
      } while (records_left);
    } while (!stop_token.stop_requested());   // hang around in case we get more work to do in future

    if (records_sent) {
      std::cout << records_sent << " total events sent to the Giopler system" << std::endl;
    }
  }};
};

// -----------------------------------------------------------------------------
static inline SinkManager g_sink_manager{};

// -----------------------------------------------------------------------------
}   // namespace giopler::sink

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_SINK_HPP
