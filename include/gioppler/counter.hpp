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
#ifndef GIOPPLER_COUNTER_HPP
#define GIOPPLER_COUNTER_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <memory>
#include <set>
#include <source_location>
#include <utility>
#include <vector>

#include <gioppler/record.hpp>
#include <gioppler/sink.hpp>
#include <gioppler/histogram.hpp>

// -----------------------------------------------------------------------------
namespace gioppler::prof {

// -----------------------------------------------------------------------------
struct PlatformCounters
{
  virtual ~PlatformCounters() = default;

  virtual std::set<std::string> get_counter_categories() = 0;
  virtual std::vector<uint64_t> get_counter_values() = 0;

  std::vector<std::string> get_counter_names() {
    static const std::vector<std::string> keys = get_keys_matching_categories(get_counter_categories());
    return keys;
  }

  std::size_t get_num_counters() {
    static const std::size_t num_counters = get_counter_names().size();
    return num_counters;
  }
};

// -----------------------------------------------------------------------------
// defined in platform file: (leave out static and extern keywords)
extern inline thread_local std::unique_ptr<PlatformCounters> g_platform_counters;

// -----------------------------------------------------------------------------
struct AggregateCounterValues
{
  AggregateCounterValues() = default;
  ~AggregateCounterValues() = default;

  uint64_t _count;
  double _workload;
  std::vector<histogram::Histogram> _counters_total;
  std::vector<histogram::Histogram> _counters_self;
};

// -----------------------------------------------------------------------------
using CounterMap = std::unordered_map<RecordCategory, AggregateCounterValues>;

// -----------------------------------------------------------------------------
/// keeps track of the lifetime of each thread
class ProfilingThread
{
 public:
  ProfilingThread() { }

  ~ProfilingThread() {

  }

  // aggregated profiling counter data for the thread
  static inline thread_local std::unique_ptr<CounterMap> g_counter_map;
};

// -----------------------------------------------------------------------------
static inline thread_local ProfilingThread g_profiling_thread;

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
class Block;

// -----------------------------------------------------------------------------
class Function
{
 public:
  explicit Function(double workload = 0,
                    const source_location& source_location =
                      source_location::current())
  : _record_category{RecordCategory(source_location, "prof", "function")},
    _workload{workload},
    _counters_subtract{g_platform_counters->get_num_counters()}
  {
    _counters_start = g_platform_counters->get_counter_values();
  }

  ~Function() {
    subtract_from_parent();

  }

 private:
  friend class Block;
  static inline thread_local std::vector<Function*> _functions;
  RecordCategory _record_category;
  double _workload;
  std::vector<uint64_t> _counters_start;      // when the function or block was entered
  std::vector<uint64_t> _counters_subtract;   // cumulative time spent in children

  void subtract_from_parent() {
    if (_functions.size() >= 2) {
      _functions[_functions.size()-2]->subtract_child();
    }
  }

  void subtract_child() {

  }
};

// -----------------------------------------------------------------------------
class Block
{
public:
  explicit Block(double workload = 0,
                 const source_location& source_location =
                   source_location::current())
  {

  }

  ~Block() {

  }

 private:
  static inline thread_local std::vector<Function*> _blocks;
};

// -----------------------------------------------------------------------------
}   // namespace gioppler::prof

// -----------------------------------------------------------------------------
#if defined(GIOPPLER_PLATFORM_LINUX)
#include "gioppler/linux/counter.hpp"
#endif

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_COUNTER_HPP
