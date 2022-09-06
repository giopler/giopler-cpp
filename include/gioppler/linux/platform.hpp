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
#ifndef GIOPPLER_LINUX_PLATFORM_HPP
#define GIOPPLER_LINUX_PLATFORM_HPP

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
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace gioppler {
uint64_t get_memory_page_size()
{
  static const auto memory_page_size = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
  return memory_page_size;
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_memory_page_size()
{
  return 4096;   // assume 4KB unless we know otherwise
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total amount of physical memory
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace gioppler {
uint64_t get_physical_memory()
{
  static const auto physical_memory =
      get_memory_page_size()*static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES));
  return physical_memory;
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_physical_memory()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total number of CPU cores configured in the system
// the actual number of available CPU cores could be less
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
// could also use sysconf(_SC_NPROCESSORS_CONF)
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/sysinfo.h>
namespace gioppler {
uint64_t get_total_cpu_cores()
{
  static const auto total_cpu_cores = static_cast<uint64_t>(get_nprocs_conf());
  return total_cpu_cores;
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_total_cpu_cores()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total number of CPU cores configured in the system
// the actual number of available CPU cores could be less
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
// could also use sysconf(_SC_NPROCESSORS_ONLN)
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/sysinfo.h>
namespace gioppler {
uint64_t get_available_cpu_cores()
{
  static const auto available_cpu_cores = static_cast<uint64_t>(get_nprocs());
  return available_cpu_cores;
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_available_cpu_cores()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Program name
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <cerrno>
namespace gioppler {
std::string get_program_name()
{
  return program_invocation_short_name;
}
}   // namespace gioppler
#else
namespace gioppler {
std::string get_program_name()
{
  return "unknown";
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Process id
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace gioppler {
uint64_t get_process_id()
{
  return getpid();
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_process_id()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Real username
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
namespace gioppler {
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
}   // namespace gioppler
#else
namespace gioppler {
std::string get_real_username()
{
  return ""s;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Effective username
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
namespace gioppler {
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
}   // namespace gioppler
#else
namespace gioppler {
std::string get_effective_username()
{
  return ""s;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// CPU Architecture
// x86_64, i686, i386, aarch64, arm
// https://stackoverflow.com/questions/45125516/possible-values-for-uname-m
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <sys/utsname.h>
namespace gioppler {
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
}   // namespace gioppler
#else
namespace gioppler {
std::string get_architecture()
{
  return ""s;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// Thread id
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <unistd.h>
namespace gioppler {
uint64_t get_thread_id()
{
  return gettid();
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_thread_id()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// NUMA node id
// this is the CPU core id where the calling thread is currently running
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
namespace gioppler {
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
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_node_id()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// CPU id
// this is the CPU core id where the calling thread is currently running
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
namespace gioppler {
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
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_cpu_id()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
/// total amount of currently available memory
// we use the value reported by the system
// https://www.gnu.org/software/libc/manual/html_node/Sysconf.html
#if defined(GIOPPLER_PLATFORM_LINUX)      // Linux kernel; could be GNU or Android
#include <unistd.h>
namespace gioppler {
uint64_t get_available_memory()
{
  static const auto available_memory =
      get_memory_page_size()*static_cast<uint64_t>(sysconf(_SC_AVPHYS_PAGES));
  return available_memory;
}
}   // namespace gioppler
#else
namespace gioppler {
uint64_t get_available_memory()
{
  return 0;
}
}   // namespace gioppler
#endif   // defined GIOPPLER_PLATFORM_LINUX

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_LINUX_PLATFORM_HPP
