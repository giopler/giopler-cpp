#include <atomic>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <cstdlib>
#include <cstring>
#include <cerrno>

#if defined(__linux__)   // Linux PMC includes
#include <linux/hw_breakpoint.h> /* Definition of HW_* constants */
#include <linux/perf_event.h>    /* Definition of PERF_* constants */
#include <sys/ioctl.h>
#include <sys/syscall.h> /* Definition of SYS_* constants */
// #include <unistd.h>
#endif

namespace brainyguy {
  enum class BuildMode { off, debug, test, qa, profile, release };
  constexpr auto BG_BUILD_MODE = BuildMode::profile;

  // ---------------------------------------------------------------------------
  class Program
  {
   public:
    explicit Program() {
      std::cerr << "inside Program()" << std::endl;
    }

    ~Program() {
      std::cerr << "inside ~Program()" << std::endl;
    }

    static void check_create() {
      std::call_once(_once_flag_create, call_once_create);
    }

    static void check_destroy() {
      std::call_once(_once_flag_destroy, call_once_destroy);
    }

   private:
    static inline std::once_flag _once_flag_create, _once_flag_destroy;
    static inline Program* _p_program;

    static void call_once_create() {
      _p_program = new Program;
    }

    static void call_once_destroy() {
      delete _p_program;
    }
  };

  // ---------------------------------------------------------------------------
  // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
  template <typename T, typename... Rest>
  void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
  {
      seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      (hash_combine(seed, rest), ...);
  }

  struct pair_hash
  {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const
    {
      std::size_t seed = 0;
      hash_combine(seed, pair.first, pair.second);
      return seed;
    }
  };

  // ---------------------------------------------------------------------------
  class Thread
  {
   public:
    explicit Thread() {
      const uint_fast64_t prev_threads_created = std::atomic_fetch_add(&_threads_created, 1);
      _thread_id = prev_threads_created+1;
      std::atomic_fetch_add(&_threads_active, 1);
      std::cerr << "inside Thread(" << _thread_id << ")" << std::endl;
    }

    ~Thread() {
      std::cerr << "inside ~Thread" << std::endl;
      std::atomic_fetch_sub(&_threads_active, 1);
    }

    static void check_create() {
      if (_p_thread == nullptr) {
        _p_thread = new Thread;
      }
    }

    static void destroy() {
      delete _p_thread;
    }

    static bool all_threads_done() {
      std::cerr << "threads active: " << _threads_active << std::endl;
      return _threads_active == 0;
    }

   private:
    static thread_local inline Thread* _p_thread;
    static inline std::atomic_uint_fast64_t _threads_created;
    static inline std::atomic_uint_fast64_t _threads_active;
    uint_fast64_t _thread_id;
  };

  // ---------------------------------------------------------------------------
  class LinuxEvent
  {
   public:
    LinuxEvent() : _fd(-1) {}

    explicit LinuxEvent(uint32_t event_type, uint64_t event, LinuxEvent group_event = LinuxEvent())
    {
      struct perf_event_attr perf_event_attr{};

      perf_event_attr.size = sizeof(perf_event_attr);
      perf_event_attr.type = event_type;
      perf_event_attr.config = event;
      perf_event_attr.disabled = 1;
      perf_event_attr.exclude_kernel = 1;
      perf_event_attr.exclude_hv = 1;

      _fd = perf_event_open(&perf_event_attr, 0, -1, group_event._fd, 0);
      if (_fd == -1) {
        std::cerr << "ERROR: LinuxEvent::constructor: " << std::strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }

    ~LinuxEvent() {
      if (_fd != -1) {
        const int ioctl_status = ioctl(_fd, PERF_EVENT_IOC_DISABLE, 0);
        if (ioctl_status == -1) {
          std::cerr << "ERROR: LinuxEvent::destructor: ioctl: " << std::strerror(errno) << std::endl;
          std::exit(EXIT_FAILURE);
        }

        const int close_status = close(_fd);
        if (close_status == -1) {
          std::cerr << "ERROR: LinuxEvent::destructor: close: " << std::strerror(errno) << std::endl;
          std::exit(EXIT_FAILURE);
        }
      }
    }

    void reset() {
      const int status = ioctl(_fd, PERF_EVENT_IOC_RESET, 0);
      if (status == -1) {
        std::cerr << "ERROR: LinuxEvent::reset: " << std::strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }

    uint64_t read_event()
    {
      uint64_t count;
      const ssize_t bytes_read = read(_fd, &count, sizeof(count));
      if (bytes_read == -1) {
        std::cerr << "ERROR: LinuxEvent::read: " << std::strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
      }
      return count;
    }

   private:
      int _fd;

    static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                               int group_fd, unsigned long flags)
    {
      return (int) syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    }
  };

  // ---------------------------------------------------------------------------
  class LinuxEventsRecord
  {
   public:
    explicit LinuxEventsRecord() = default;

    LinuxEventsRecord& operator+=(const LinuxEventsRecord& rhs)
    {
      _fd_sw_cpu_clock += rhs._fd_sw_cpu_clock;
      _fd_sw_task_clock += rhs._fd_sw_task_clock;
      _fd_sw_page_faults += rhs._fd_sw_page_faults;
      _fd_sw_context_switches += rhs._fd_sw_context_switches;
      _fd_sw_cpu_migrations += rhs._fd_sw_cpu_migrations;
      _fd_sw_page_faults_min += rhs._fd_sw_page_faults_min;
      _fd_sw_page_faults_maj += rhs._fd_sw_page_faults_maj;
      _fd_sw_alignment_faults += rhs._fd_sw_alignment_faults;
      _fd_sw_emulation_faults += rhs._fd_sw_emulation_faults;
      _fd_hw_cpu_cycles += rhs._fd_hw_cpu_cycles;
      _fd_hw_instructions += rhs._fd_hw_instructions;
      _fd_hw_cache_references += rhs._fd_hw_cache_references;
      _fd_hw_cache_misses += rhs._fd_hw_cache_misses;
      _fd_hw_branch_instructions += rhs._fd_hw_branch_instructions;
      _fd_hw_branch_misses += rhs._fd_hw_branch_misses;
      _fd_hw_stalled_cycles_frontend += rhs._fd_hw_stalled_cycles_frontend;
      _fd_hw_stalled_cycles_backend += rhs._fd_hw_stalled_cycles_backend;
      return *this;
    }

    friend LinuxEventsRecord operator+(LinuxEventsRecord lhs, const LinuxEventsRecord& rhs)
    {
      lhs += rhs;
      return lhs;
    }

    LinuxEventsRecord& operator-=(const LinuxEventsRecord& rhs)
    {
      _fd_sw_cpu_clock -= rhs._fd_sw_cpu_clock;
      _fd_sw_task_clock -= rhs._fd_sw_task_clock;
      _fd_sw_page_faults -= rhs._fd_sw_page_faults;
      _fd_sw_context_switches -= rhs._fd_sw_context_switches;
      _fd_sw_cpu_migrations -= rhs._fd_sw_cpu_migrations;
      _fd_sw_page_faults_min -= rhs._fd_sw_page_faults_min;
      _fd_sw_page_faults_maj -= rhs._fd_sw_page_faults_maj;
      _fd_sw_alignment_faults -= rhs._fd_sw_alignment_faults;
      _fd_sw_emulation_faults -= rhs._fd_sw_emulation_faults;
      _fd_hw_cpu_cycles -= rhs._fd_hw_cpu_cycles;
      _fd_hw_instructions -= rhs._fd_hw_instructions;
      _fd_hw_cache_references -= rhs._fd_hw_cache_references;
      _fd_hw_cache_misses -= rhs._fd_hw_cache_misses;
      _fd_hw_branch_instructions -= rhs._fd_hw_branch_instructions;
      _fd_hw_branch_misses -= rhs._fd_hw_branch_misses;
      _fd_hw_stalled_cycles_frontend -= rhs._fd_hw_stalled_cycles_frontend;
      _fd_hw_stalled_cycles_backend -= rhs._fd_hw_stalled_cycles_backend;
      return *this;
    }

    friend LinuxEventsRecord operator-(LinuxEventsRecord lhs, const LinuxEventsRecord& rhs)
    {
      lhs -= rhs;
      return lhs;
    }

    uint64_t _fd_sw_cpu_clock;         // This reports the CPU clock, a high-resolution per-CPU timer.
    uint64_t _fd_sw_task_clock;        // This reports a clock count specific to the task that is running.
    uint64_t _fd_sw_page_faults;       // This reports the number of page faults.
    uint64_t _fd_sw_context_switches;  // This counts context switches.
    uint64_t _fd_sw_cpu_migrations;    // This reports the number of times the process has migrated to a new CPU.
    uint64_t _fd_sw_page_faults_min;   // This counts the number of minor page faults.
    uint64_t _fd_sw_page_faults_maj;   // This counts the number of major page faults. These required disk I/O to handle.
    uint64_t _fd_sw_alignment_faults;  // This counts the number of alignment faults.
    uint64_t _fd_sw_emulation_faults;  // This  counts the number of emulation faults.

    uint64_t _fd_hw_cpu_cycles;        // Total cycles.
    uint64_t _fd_hw_instructions;      // Retired instructions.

    uint64_t _fd_hw_cache_references;  // Cache accesses.  Usually this indicates Last Level Cache accesses.
    uint64_t _fd_hw_cache_misses;      // Cache misses.  Usually this indicates Last Level Cache misses.

    uint64_t _fd_hw_branch_instructions; // Retired branch instructions.
    uint64_t _fd_hw_branch_misses;       // Mispredicted branch instructions.

    uint64_t _fd_hw_stalled_cycles_frontend; // Stalled cycles during issue.
    uint64_t _fd_hw_stalled_cycles_backend;  // Stalled cycles during retirement.
  };

  // ---------------------------------------------------------------------------
  class LinuxEvents
  {
    public:
      explicit LinuxEvents()
      {
        open_events();
      }

      static void check_create() {
        if (_p_linux_events == nullptr) {
          _p_linux_events = new LinuxEvents;
        }
      }

      static void destroy() {
        delete _p_linux_events;
      }

      LinuxEventsRecord get_snapshot()
      {
        LinuxEventsRecord record{};
        record._fd_sw_cpu_clock = _fd_sw_cpu_clock.read_event();
        record._fd_sw_task_clock = _fd_sw_task_clock.read_event();
        record._fd_sw_page_faults = _fd_sw_page_faults.read_event();
        record._fd_sw_context_switches = _fd_sw_context_switches.read_event();
        record._fd_sw_cpu_migrations = _fd_sw_cpu_migrations.read_event();
        record._fd_sw_page_faults_min = _fd_sw_page_faults_min.read_event();
        record._fd_sw_page_faults_maj = _fd_sw_page_faults_maj.read_event();
        record._fd_sw_alignment_faults = _fd_sw_alignment_faults.read_event();
        record._fd_sw_emulation_faults = _fd_sw_emulation_faults.read_event();

        record._fd_hw_cpu_cycles = _fd_hw_cpu_cycles.read_event();
        record._fd_hw_instructions = _fd_hw_instructions.read_event();
        record._fd_hw_cache_references = _fd_hw_cache_references.read_event();
        record._fd_hw_cache_misses = _fd_hw_cache_misses.read_event();
        record._fd_hw_branch_instructions = _fd_hw_branch_instructions.read_event();
        record._fd_hw_branch_misses = _fd_hw_branch_misses.read_event();
        record._fd_hw_stalled_cycles_frontend = _fd_hw_stalled_cycles_frontend.read_event();
        record._fd_hw_stalled_cycles_backend = _fd_hw_stalled_cycles_backend.read_event();
        return record;
      }

    private:
      static thread_local inline LinuxEvents* _p_linux_events;

      LinuxEvent _fd_sw_cpu_clock;         // This reports the CPU clock, a high-resolution per-CPU timer.
      LinuxEvent _fd_sw_task_clock;        // This reports a clock count specific to the task that is running.
      LinuxEvent _fd_sw_page_faults;       // This reports the number of page faults.
      LinuxEvent _fd_sw_context_switches;  // This counts context switches.
      LinuxEvent _fd_sw_cpu_migrations;    // This reports the number of times the process has migrated to a new CPU.
      LinuxEvent _fd_sw_page_faults_min;   // This counts the number of minor page faults.
      LinuxEvent _fd_sw_page_faults_maj;   // This counts the number of major page faults. These required disk I/O to handle.
      LinuxEvent _fd_sw_alignment_faults;  // This counts the number of alignment faults.
      LinuxEvent _fd_sw_emulation_faults;  // This  counts the number of emulation faults.

      LinuxEvent _fd_hw_cpu_cycles;        // Total cycles.
      LinuxEvent _fd_hw_instructions;      // Retired instructions.

      LinuxEvent _fd_hw_cache_references;  // Cache accesses.  Usually this indicates Last Level Cache accesses.
      LinuxEvent _fd_hw_cache_misses;      // Cache misses.  Usually this indicates Last Level Cache misses.

      LinuxEvent _fd_hw_branch_instructions; // Retired branch instructions.
      LinuxEvent _fd_hw_branch_misses;       // Mispredicted branch instructions.

      LinuxEvent _fd_hw_stalled_cycles_frontend; // Stalled cycles during issue.
      LinuxEvent _fd_hw_stalled_cycles_backend;  // Stalled cycles during retirement.

    void open_events() {
      _fd_sw_cpu_clock = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK);
      _fd_sw_task_clock = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK);
      _fd_sw_page_faults = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS);
      _fd_sw_context_switches = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES);
      _fd_sw_cpu_migrations = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS);
      _fd_sw_page_faults_min = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN);
      _fd_sw_page_faults_maj = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ);
      _fd_sw_alignment_faults = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS);
      _fd_sw_emulation_faults = LinuxEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS);

      _fd_hw_cpu_cycles = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
      _fd_hw_instructions = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, _fd_hw_cpu_cycles);
      _fd_hw_cache_references = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES);
      _fd_hw_cache_misses = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, _fd_hw_cache_references);
      _fd_hw_branch_instructions = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
      _fd_hw_branch_misses = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, _fd_hw_branch_instructions);
      _fd_hw_stalled_cycles_frontend = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND);
      _fd_hw_stalled_cycles_backend = LinuxEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND, _fd_hw_stalled_cycles_frontend);
    }
  };

  // ---------------------------------------------------------------------------
  class ProfileRecord {
   public:
    explicit ProfileRecord(const std::string_view function_signature,
                           const std::string_view parent_function_signature)
                           :
                           _function_signature(function_signature),
                           _parent_function_signature(parent_function_signature)
    {
    }

    struct hash
    {
        std::size_t operator()(ProfileRecord const& profile_record) const noexcept
        {
          std::size_t seed = 0;
          hash_combine(seed, profile_record._function_signature,
                       profile_record._parent_function_signature);
          return seed;
        }
    };

   private:
    const std::string_view _function_signature;
    const std::string_view _parent_function_signature;
  };

  // ---------------------------------------------------------------------------
  template <BuildMode build_mode = BG_BUILD_MODE>
  class Function
  {
  public:
    explicit Function([[maybe_unused]] const std::string_view subsystem = "",
                      [[maybe_unused]] const double count = 0.0,
                      [[maybe_unused]] std::string session = "",
                      [[maybe_unused]] const source_location& location =
                          source_location::current())
      requires (build_mode == BuildMode::off)
    {
      std::cerr << "inside Function() none" << std::endl;
    }

    explicit Function(const std::string_view subsystem = "",
                      const double count = 0.0,
                      std::string session = "",
                      const source_location& location = source_location::current())
      requires (build_mode == BuildMode::profile)
    {
      check_create_program_thread();
      std::cerr << "inside Function() profile" << std::endl;

      std::cout << "file: "
		<< location.file_name() << "("
		<< location.line() << ":"
		<< location.function_name() << "`: "
		<< subsystem << ": "
		<< count << ": "
		<< session << std::endl;
    }

    ~Function()
    {
      std::cerr << "inside ~Function()" << std::endl;
      check_destroy_program_thread();
    }

   private:
    static thread_local inline std::stack<Function<BG_BUILD_MODE>> _functions;
    static thread_local inline std::stack<std::string> _subsystems;
    static thread_local inline std::stack<std::string> _sessions;

    void check_create_program_thread()
    {
      Program::check_create();
      Thread::check_create();
    }

    void check_destroy_program_thread()
    {
      if (g_function == nullptr) {
        Thread::destroy();
      }
      if (Thread::all_threads_done()) {
        Program::check_destroy();
      }
    }
  };

  // std::unordered_map<pair, int, pair_hash> unordered_map

//    for (auto const &entry: unordered_map)
//    {
//        auto key_pair = entry.first;
//        std::cout << "{" << key_pair.first << "," << key_pair.second << "}, "
//                  << entry.second << std::endl;
//    }

}

int test(const int instance)
{
  brainyguy::Function f{"test"};
  std::cerr << "inside test " << instance << std::endl;
  if (instance > 1) {
    test(instance-1);
  }
  return 0;
}

int
main() {
  brainyguy::Function f;

  //const int t1_result = test();
  std::thread t2 = std::thread(test, 1);
  std::thread t3 = std::thread(test, 2);
  std::thread t4 = std::thread(test, 3);
  t2.join();
  t3.join();
  t4.join();
}
