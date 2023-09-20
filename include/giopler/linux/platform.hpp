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
#ifndef GIOPLER_LINUX_PLATFORM_HPP
#define GIOPLER_LINUX_PLATFORM_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <string>

// -----------------------------------------------------------------------------
/// memory page size
// we use the value reported by the system
// in theory we could have more than one page size
// the value could change as the operating system runs, but should be a constant for the process
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace giopler {
uint64_t get_memory_page_size()
{
  static const auto memory_page_size = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
  return memory_page_size;
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_memory_page_size()
{
  return 4096;   // assume 4KB unless we know otherwise
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total amount of physical memory
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace giopler {
uint64_t get_physical_memory()
{
  static const auto physical_memory =
      get_memory_page_size()*static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES));
  return physical_memory;
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_physical_memory()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total number of CPU cores configured in the system
// the actual number of available CPU cores could be less
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
// could also use sysconf(_SC_NPROCESSORS_CONF)
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/sysinfo.h>
namespace giopler {
uint64_t get_conf_cpu_cores()
{
  static const auto total_cpu_cores = static_cast<uint64_t>(get_nprocs_conf());
  return total_cpu_cores;
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_conf_cpu_cores()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total number of CPU cores available in the system
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
// could also use sysconf(_SC_NPROCESSORS_ONLN)
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/sysinfo.h>
namespace giopler {
uint64_t get_available_cpu_cores()
{
  static const auto available_cpu_cores = static_cast<uint64_t>(get_nprocs());
  return available_cpu_cores;
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_available_cpu_cores()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Program name
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <cerrno>
namespace giopler {
std::string get_program_name()
{
  return program_invocation_short_name;
}
}   // namespace giopler
#else
namespace giopler {
std::string get_program_name()
{
  return "unknown";
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Process id
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace giopler {
uint64_t get_process_id()
{
  return getpid();
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_process_id()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// CPU Architecture
// x86_64, i686, i386, aarch64, arm
// https://stackoverflow.com/questions/45125516/possible-values-for-uname-m
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/utsname.h>
namespace giopler {
std::string get_architecture()
{
  struct utsname uts_name{};
  const int status = uname(&uts_name);
  if (status == 0) {
    return uts_name.machine;
  } else {
    return ""s;
  }
}
}   // namespace giopler
#else
namespace giopler {
std::string get_architecture()
{
  return ""s;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// System host name
#if defined(GIOPLER_PLATFORM_LINUX)     // Linux kernel; could be GNU or Android
#include <unistd.h>
#include <climits>                      // HOST_NAME_MAX
namespace giopler {
std::string get_host_name()
{
  char host_name[HOST_NAME_MAX];
  const int status = gethostname(host_name, HOST_NAME_MAX);
  if (status == 0) {
    host_name[HOST_NAME_MAX-1] = '\0';   // make sure it is null-terminated
    return host_name;
  } else {
    return ""s;
  }
}
}   // namespace giopler
#else
namespace giopler {
std::string get_host_name()
{
  return ""s;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Real username
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
namespace giopler {
std::string get_real_username()
{
  const uid_t user_id{getuid()};
  const struct passwd * password_entry = getpwuid(user_id);
  if (password_entry) {
    return password_entry->pw_name;
  } else {
    return ""s;
  }
}
}   // namespace giopler
#else
namespace giopler {
std::string get_real_username()
{
  return ""s;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Effective username
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
namespace giopler {
std::string get_effective_username()
{
  const uid_t effective_user_id{geteuid()};
  const struct passwd * password_entry = getpwuid(effective_user_id);
  if (password_entry) {
    return password_entry->pw_name;
  } else {
    return ""s;
  }
}
}   // namespace giopler
#else
namespace giopler {
std::string get_effective_username()
{
  return ""s;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Thread id
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <unistd.h>
namespace giopler {
uint64_t get_thread_id()
{
  return gettid();
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_thread_id()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// NUMA node id
// this is the NUMA node id where the calling thread is currently running
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
namespace giopler {
uint64_t get_node_id()
{
  unsigned int node;
  const int status = getcpu(NULL, &node);
  if (status == 0) {
    return static_cast<uint64_t>(node);
  } else {
    return 0;
  }
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_node_id()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// CPU id
// this is the CPU core id where the calling thread is currently running
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
namespace giopler {
uint64_t get_cpu_id()
{
  unsigned int cpu;
  const int status = getcpu(&cpu, NULL);
  if (status == 0) {
    return static_cast<uint64_t>(cpu);
  } else {
    return 0;
  }
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_cpu_id()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total amount of currently available memory
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace giopler {
uint64_t get_available_memory()
{
  static const auto available_memory =
      get_memory_page_size()*static_cast<uint64_t>(sysconf(_SC_AVPHYS_PAGES));
  return available_memory;
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_available_memory()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// current frequency in kHz for the current CPU core
// we use the value reported by the system
// https://www.kernel.org/doc/html/v6.5/admin-guide/pm/cpufreq.html
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <fstream>
#include <string>
namespace giopler {
uint64_t get_cur_freq()
{
  const uint64_t cpu_id = get_cpu_id();
  std::ifstream freq_file("/sys/devices/system/cpu/cpu" + std::to_string(cpu_id) + "/cpufreq/scaling_cur_freq");
  if (!freq_file.is_open()) {
    return 0;
  }

  std::string freq_str;
  freq_file >> freq_str;
  return std::stoul(freq_str);
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_cur_freq()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// maximum frequency in kHz for the current CPU core
// we use the value reported by the system
// https://www.kernel.org/doc/html/v6.5/admin-guide/pm/cpufreq.html
// Perf and Eff cores in modern CPUs have different max frequencies
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <fstream>
#include <string>
namespace giopler {
uint64_t get_max_freq()
{
  const uint64_t cpu_id = get_cpu_id();
  std::ifstream freq_file("/sys/devices/system/cpu/cpu" + std::to_string(cpu_id) + "/cpufreq/scaling_max_freq");
  if (!freq_file.is_open()) {
    return 0;
  }

  std::string freq_str;
  freq_file >> freq_str;
  return std::stoul(freq_str);
}
}   // namespace giopler
#else
namespace giopler {
uint64_t get_max_freq()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// system CPU load average / available CPU cores
// we use the values reported by the system
// https://man7.org/linux/man-pages/man3/getloadavg.3.html
#if defined(GIOPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <cstdlib>
namespace giopler {
double get_load_average1()
{
  double loads[3];
  const int status = getloadavg(loads, 3);
  return (status == -1) ? 0 : (loads[0] / get_available_cpu_cores());
}
double get_load_average5()
{
  double loads[3];
  const int status = getloadavg(loads, 3);
  return (status == -1) ? 0 : (loads[1] / get_available_cpu_cores());
}
double get_load_average15()
{
  double loads[3];
  const int status = getloadavg(loads, 3);
  return (status == -1) ? 0 : (loads[2] / get_available_cpu_cores());
}
}   // namespace giopler
#else
namespace giopler {
double get_load_average1()
{
  return 0;
}
double get_load_average5()
{
  return 0;
}
double get_load_average15()
{
  return 0;
}
}   // namespace giopler
#endif   // defined GIOPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_LINUX_PLATFORM_HPP
