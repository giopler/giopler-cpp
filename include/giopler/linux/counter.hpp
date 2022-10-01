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
#ifndef GIOPLER_LINUX_COUNTER_HPP
#define GIOPLER_LINUX_COUNTER_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <math.h>
#include "giopler/record.hpp"

// -----------------------------------------------------------------------------
/// Performance Monitoring Counters (pmc), a Linux feature.
#include <linux/perf_event.h>             // Definition of PERF_* constants
#include <linux/hw_breakpoint.h>          // Definition of HW_* constants
#include <sys/syscall.h>                  // Definition of SYS_* constants
#include <sys/ioctl.h>
#include <unistd.h>

// -----------------------------------------------------------------------------
namespace giopler::dev
{

// -----------------------------------------------------------------------------
class LinuxEvent {
 public:
  LinuxEvent() : _num_events{} { }

  LinuxEvent(const std::string_view name, const uint32_t event_type, const uint64_t event) : _num_events{1} {
    _name1 = name;
    _fd1   = open_event(name, event_type, event, -1);
  }

  LinuxEvent(const std::string_view name1, const uint32_t event_type1, const uint64_t event1,
             const std::string_view name2, const uint32_t event_type2, const uint64_t event2) : _num_events{2} {
    _name1 = name1;
    _fd1   = open_event(name1, event_type1, event1, -1);
    _name2 = name2;
    _fd2   = open_event(name2, event_type2, event2, _fd1);

    reset_event(name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));
  }

  LinuxEvent(const std::string_view name1, const uint32_t event_type1, const uint64_t event1,
             const std::string_view name2, const uint32_t event_type2, const uint64_t event2,
             const std::string_view name3, const uint32_t event_type3, const uint64_t event3) : _num_events{3} {
    _name1 = name1;
    _fd1   = open_event(name1, event_type1, event1, -1);
    _name2 = name2;
    _fd2   = open_event(name2, event_type2, event2, _fd1);
    _name3 = name3;
    _fd3   = open_event(name3, event_type3, event3, _fd1);

    reset_event(name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));
  }

  LinuxEvent(const std::string_view name1, const uint32_t event_type1, const uint64_t event1,
             const std::string_view name2, const uint32_t event_type2, const uint64_t event2,
             const std::string_view name3, const uint32_t event_type3, const uint64_t event3,
             const std::string_view name4, const uint32_t event_type4, const uint64_t event4) : _num_events{4} {
    _name1 = name1;
    _fd1   = open_event(name1, event_type1, event1, -1);
    _name2 = name2;
    _fd2   = open_event(name2, event_type2, event2, _fd1);
    _name3 = name3;
    _fd3   = open_event(name3, event_type3, event3, _fd1);
    _name4 = name4;
    _fd4   = open_event(name4, event_type4, event4, _fd1);

    reset_event(name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));
  }

  ~LinuxEvent() {
    disable_event(_name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));

    switch (_num_events) {
      case 4: close_event(_name4, _fd4);   /* fall-through */
      case 3: close_event(_name3, _fd3);   /* fall-through */
      case 2: close_event(_name2, _fd2);   /* fall-through */
      case 1: close_event(_name1, _fd1);
    }
  }

  void reset_events() {
    reset_event(_name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));
  }

  void enable_events() {
    enable_event(_name1, _fd1, (_num_events == 1 ? Group::single : Group::leader));
  }

  int64_t read_event() {
    return read_event1();
  }

  int64_t read_event1() {
    assert(_num_events >= 1);
    return read_event(_name1, _fd1);
  }

  int64_t read_event2() {
    assert(_num_events >= 2);
    return read_event(_name2, _fd2);
  }

  int64_t read_event3() {
    assert(_num_events >= 3);
    return read_event(_name3, _fd3);
  }

  int64_t read_event4() {
    assert(_num_events >= 4);
    return read_event(_name4, _fd4);
  }

 private:
  enum class Group { leader, single };
  const int _num_events;
  std::string_view _name1, _name2, _name3, _name4;
  int _fd1, _fd2, _fd3, _fd4;

  // measures the calling process/thread on any CPU
  // state saved/restored on context switch
  static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                             int group_fd, unsigned long flags) {
    return static_cast<int>(syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags));
  }

  static int open_event(const std::string_view name,
                        const uint32_t event_type, const uint64_t event, const int group_fd) {
    struct perf_event_attr perf_event_attr{};
    perf_event_attr.size = sizeof(perf_event_attr);
    perf_event_attr.type = event_type;
    perf_event_attr.config = event;
    perf_event_attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    perf_event_attr.disabled = 1;
    perf_event_attr.exclude_kernel = 1;
    perf_event_attr.exclude_hv = 1;

    const int fd = perf_event_open(&perf_event_attr, 0, -1, group_fd, 0);
    if (fd == -1) {
      std::cerr << "ERROR: LinuxEvent::open_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }

    return fd;
  }

  static void reset_event(const std::string_view name, const int fd, const Group group) {
    const int status = ioctl(fd, PERF_EVENT_IOC_RESET, (group == Group::leader ? PERF_IOC_FLAG_GROUP : 0));
    if (status == -1) {
      std::cerr << "ERROR: LinuxEvent::reset_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  static void disable_event(const std::string_view name, const int fd, const Group group) {
    const int status = ioctl(fd, PERF_EVENT_IOC_DISABLE, (group == Group::leader ? PERF_IOC_FLAG_GROUP : 0));
    if (status == -1) {
      std::cerr << "ERROR: LinuxEvent::disable_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  static void enable_event(const std::string_view name, const int fd, const Group group) {
    const int status = ioctl(fd, PERF_EVENT_IOC_ENABLE, (group == Group::leader ? PERF_IOC_FLAG_GROUP : 0));
    if (status == -1) {
      std::cerr << "ERROR: LinuxEvent::enable_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  static void close_event(const std::string_view name, const int fd) {
    const int status = close(fd);
    if (status == -1) {
      std::cerr << "ERROR: LinuxEvent::close_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  /// read the counter value
  // the value is scaled to account for performance counter multiplexing
  static int64_t read_event(const std::string_view name, const int fd) {
    struct ReadData {
      uint64_t value;         /* The value of the event */
      uint64_t time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
      uint64_t time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
      uint64_t id;            /* if PERF_FORMAT_ID */
    };

    ReadData read_data{};
    const ssize_t bytes_read = read(fd, &read_data, sizeof(read_data));
    if (bytes_read == -1) {
      std::cerr << "ERROR: LinuxEvent::read_event: " << name << ": " << std::strerror(errno) << std::endl;
      std::exit(EXIT_FAILURE);
    }

    if (read_data.time_enabled && read_data.time_running) {
      const double active_pct = static_cast<double>(read_data.time_running) / static_cast<double>(read_data.time_enabled);
      const std::int64_t scaled_counter = std::lround(static_cast<double>(read_data.value) * (1.0 / active_pct));
      // std::cout << name << " = " << scaled_counter << " <= " << read_data.value
      //           << " (" << read_data.time_running << "/" << read_data.time_enabled << ")" << std::endl;
      return scaled_counter;
    } else {
      return 0;
    }
  }
};

// -----------------------------------------------------------------------------
class LinuxEvents {
 public:
  explicit LinuxEvents() {
    if constexpr (g_build_mode == BuildMode::Prof) {
      open_events();
      enable_events();
    }
  }

  ~LinuxEvents() = default;   // events are closed when the objects are destroyed

  void enable_events() {
    _fd_sw_cpu_clock->enable_events();
    _fd_sw_task_clock->enable_events();
    _fd_sw_page_faults->enable_events();
    _fd_sw_context_switches->enable_events();
    _fd_sw_cpu_migrations->enable_events();
    _fd_sw_page_faults_min->enable_events();
    _fd_sw_page_faults_maj->enable_events();
    _fd_sw_alignment_faults->enable_events();
    _fd_sw_emulation_faults->enable_events();

    _fd_hw_cpu_cycles_instr_group->enable_events();
    _fd_hw_cpu_stalled_cycles_group->enable_events();
    _fd_hw_branch_instructions_misses_group->enable_events();
    _fd_hw_cache_references_misses_group->enable_events();
  }

  /// get the current values of the performance counters
  Record get_snapshot() {
    return Record({
      {"prof.sw.cpu_clock"s,        ns_to_sec(_fd_sw_cpu_clock->read_event())},
      {"prof.sw.task_clock"s,       ns_to_sec(_fd_sw_task_clock->read_event())},
      {"prof.sw.page_faults"s,      _fd_sw_page_faults->read_event()},
      {"prof.sw.context_switches"s, _fd_sw_context_switches->read_event()},
      {"prof.sw.cpu_migrations"s,   _fd_sw_cpu_migrations->read_event()},
      {"prof.sw.page_faults_min"s,  _fd_sw_page_faults_min->read_event()},
      {"prof.sw.page_faults_maj"s,  _fd_sw_page_faults_maj->read_event()},
      {"prof.sw.alignment_faults"s, _fd_sw_alignment_faults->read_event()},
      {"prof.sw.emulation_faults"s, _fd_sw_emulation_faults->read_event()},

      {""s, _fd_hw_cpu_cycles_instr_group->read_event1()},
      {""s, _fd_hw_cpu_cycles_instr_group->read_event2()},

      {""s, _fd_hw_cpu_stalled_cycles_group->read_event1()},
      {""s, _fd_hw_cpu_stalled_cycles_group->read_event2()},

      {""s, _fd_hw_branch_instructions_misses_group->read_event1()},
      {""s, _fd_hw_branch_instructions_misses_group->read_event2()},

      {""s, _fd_hw_cache_references_misses_group->read_event1()},
      {""s, _fd_hw_cache_references_misses_group->read_event2()}

    });
  }

 private:
  std::unique_ptr<LinuxEvent> _fd_sw_cpu_clock;         // This reports the CPU clock, a high-resolution per-CPU timer.
  std::unique_ptr<LinuxEvent> _fd_sw_task_clock;        // This reports a clock count specific to the task that is running.
  std::unique_ptr<LinuxEvent> _fd_sw_page_faults;       // This reports the number of page faults.
  std::unique_ptr<LinuxEvent> _fd_sw_context_switches;  // This counts context switches.
  std::unique_ptr<LinuxEvent> _fd_sw_cpu_migrations;    // This reports the number of times the process has migrated to a new CPU.
  std::unique_ptr<LinuxEvent> _fd_sw_page_faults_min;   // This counts the number of minor page faults.
  std::unique_ptr<LinuxEvent> _fd_sw_page_faults_maj;   // This counts the number of major page faults. These required disk I/O to handle.
  std::unique_ptr<LinuxEvent> _fd_sw_alignment_faults;  // This counts the number of alignment faults.
  std::unique_ptr<LinuxEvent> _fd_sw_emulation_faults;  // This  counts the number of emulation faults.

  // Total cycles.
  // Retired instructions.
  std::unique_ptr<LinuxEvent> _fd_hw_cpu_cycles_instr_group;

  // Stalled cycles during issue.
  // Stalled cycles during retirement.
  std::unique_ptr<LinuxEvent> _fd_hw_cpu_stalled_cycles_group;

  // Cache accesses.  Usually this indicates Last Level Cache accesses.
  // Cache misses.  Usually this indicates Last Level Cache misses.
  std::unique_ptr<LinuxEvent> _fd_hw_cache_references_misses_group;

  // Retired branch instructions.
  // Mispredicted branch instructions.
  std::unique_ptr<LinuxEvent> _fd_hw_branch_instructions_misses_group;

  // ---------------------------------------------------------------------------
  void open_events() {
    _fd_sw_cpu_clock = std::make_unique<LinuxEvent>("PERF_COUNT_SW_CPU_CLOCK",
                                          PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK);
    _fd_sw_task_clock = std::make_unique<LinuxEvent>("PERF_COUNT_SW_TASK_CLOCK",
                                            PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK);
    _fd_sw_page_faults = std::make_unique<LinuxEvent>("PERF_COUNT_SW_PAGE_FAULTS",
                                             PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS);
    _fd_sw_context_switches = std::make_unique<LinuxEvent>("PERF_COUNT_SW_CONTEXT_SWITCHES",
                                                  PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES);
    _fd_sw_cpu_migrations = std::make_unique<LinuxEvent>("PERF_COUNT_SW_CPU_MIGRATIONS",
                                                PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS);
    _fd_sw_page_faults_min = std::make_unique<LinuxEvent>("PERF_COUNT_SW_PAGE_FAULTS_MIN",
                                                 PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN);
    _fd_sw_page_faults_maj = std::make_unique<LinuxEvent>("PERF_COUNT_SW_PAGE_FAULTS_MAJ",
                                                 PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ);
    _fd_sw_alignment_faults = std::make_unique<LinuxEvent>("PERF_COUNT_SW_ALIGNMENT_FAULTS",
                                                  PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS);
    _fd_sw_emulation_faults = std::make_unique<LinuxEvent>("PERF_COUNT_SW_EMULATION_FAULTS",
                                                  PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS);

    _fd_hw_cpu_cycles_instr_group =
        std::make_unique<LinuxEvent>("PERF_COUNT_HW_CPU_CYCLES",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES,
                                     "PERF_COUNT_HW_INSTRUCTIONS",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);

    _fd_hw_cpu_stalled_cycles_group =
        std::make_unique<LinuxEvent>("PERF_COUNT_HW_STALLED_CYCLES_FRONTEND",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND,
                                     "PERF_COUNT_HW_STALLED_CYCLES_BACKEND",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND);

    _fd_hw_branch_instructions_misses_group =
        std::make_unique<LinuxEvent>("PERF_COUNT_HW_BRANCH_INSTRUCTIONS",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
                                     "PERF_COUNT_HW_BRANCH_MISSES",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);

    _fd_hw_cache_references_misses_group =
        std::make_unique<LinuxEvent>("PERF_COUNT_HW_CACHE_REFERENCES",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES,
                                     "PERF_COUNT_HW_CACHE_MISSES",
                                     PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);

  }

  /// convert nanosecond counter to floating point seconds
  static double ns_to_sec(const std::uint64_t ns) {
    constexpr double factor = 0.000'000'001;
    return static_cast<double>(ns) * factor;
  }
};

// -----------------------------------------------------------------------------
/// open the event counters when the thread is started
static inline thread_local LinuxEvents g_linux_events;

// -----------------------------------------------------------------------------
/// read platform-specific performance event counters
// assumed to return the same set of keys on every invocation at a given platform
Record read_event_counters() {
  return g_linux_events.get_snapshot();
}

// -----------------------------------------------------------------------------
} // namespace giopler::linux

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LINUX_COUNTER_HPP
