/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <charconv>
#include <chrono>
#include <cstring>
#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#endif
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "ylt/util/dragonbox_to_chars.h"
#include "ylt/util/meta_string.hpp"
#include "ylt/util/type_traits.h"

#if defined(_WIN32)
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN  // Sorry for the inconvenience. Please include any
                             // related headers you need manually.
                             // (https://stackoverflow.com/a/8294669)
#define WIN32_LEAN_AND_MEAN  // Prevent inclusion of WinSock2.h
#endif
#include <Windows.h>  // Force inclusion of WinGDI here to resolve name conflict
#endif
#ifdef ERROR  // Should be true unless someone else undef'd it already
#undef ERROR  // Windows GDI defines this macro; make it a global enum so it
              // doesn't conflict with our code
enum { ERROR = 0 };
#endif
#endif

namespace easylog {

enum class Severity {
  NONE = 0,
  TRACE = 1,
  DEBUG = 2,
  INFO = 3,
  WARN = 4,
  ERROR = 5,
  CRITICAL = 6,
};

inline std::string_view severity_str(Severity severity) {
  switch (severity) {
    case Severity::TRACE:
      return "TRACE   ";
    case Severity::DEBUG:
      return "DEBUG   ";
    case Severity::INFO:
      return "INFO    ";
    case Severity::WARN:
      return "WARNING ";
    case Severity::ERROR:
      return "ERROR   ";
    case Severity::CRITICAL:
      return "CRITICAL";
    default:
      return "NONE    ";
  }
}

class record_t {
 public:
  record_t() = default;
  record_t(auto tm_point, Severity severity, std::string_view str)
      : tm_point_(tm_point),
        severity_(severity),
        tid_(_get_tid()),
        file_str_(str) {
    ss_.reserve(64);
  }
  record_t(record_t &&) = default;
  record_t &operator=(record_t &&) = default;

  Severity get_severity() const { return severity_; }

  const char *get_message() {
    ss_.push_back('\n');
    return ss_.data();
  }

  std::string_view get_file_str() const { return file_str_; }

  unsigned int get_tid() const { return tid_; }

  auto get_time_point() const { return tm_point_; }

  record_t &ref() { return *this; }

  template <typename T>
  record_t &operator<<(const T &data) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_floating_point_v<U>) {
      char temp[40];
      const auto end = jkj::dragonbox::to_chars(data, temp);
      ss_.append(temp, std::distance(temp, end));
    }
    else if constexpr (std::is_same_v<char, U>) {
      ss_.push_back(data);
    }
    else if constexpr (std::is_enum_v<U>) {
      int val = (int)data;
      *this << val;
    }
    else if constexpr (std::is_integral_v<U>) {
      char buf[32];
      auto [ptr, ec] = std::to_chars(buf, buf + 32, data);
      ss_.append(buf, std::distance(buf, ptr));
    }
    else {
      ss_.append(data);
    }

    return *this;
  }

  template <typename... Args>
  record_t &sprintf(const char *fmt, Args... args) {
    printf_string_format(fmt, args...);

    return *this;
  }

 private:
  template <typename... Args>
  void printf_string_format(const char *fmt, Args... args) {
    size_t size = snprintf(nullptr, 0, fmt, args...);

#ifdef YLT_ENABLE_PMR
#if __has_include(<memory_resource>)
    char arr[1024];
    std::pmr::monotonic_buffer_resource resource(arr, 1024);
    std::pmr::string buf{&resource};
#endif
#else
    std::string buf;
#endif
    buf.reserve(size + 1);
    buf.resize(size);

    snprintf(&buf[0], size + 1, fmt, args...);

    ss_.append(buf);
  }

  unsigned int _get_tid() {
    static thread_local unsigned int tid = get_tid_impl();
    return tid;
  }

  unsigned int get_tid_impl() {
#ifdef _WIN32
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
#elif defined(__linux__)
    return static_cast<unsigned int>(::syscall(__NR_gettid));
#elif defined(__FreeBSD__)
    long tid;
    syscall(SYS_thr_self, &tid);
    return static_cast<unsigned int>(tid);
#elif defined(__rtems__)
    return rtems_task_self();
#elif defined(__APPLE__)
    uint64_t tid64;
    pthread_threadid_np(NULL, &tid64);
    return static_cast<unsigned int>(tid64);
#else
    return 0;
#endif
  }

  std::chrono::system_clock::time_point tm_point_;
  Severity severity_;
  unsigned int tid_;
  std::string file_str_;

#ifdef YLT_ENABLE_PMR
#if __has_include(<memory_resource>)
  char arr_[1024];
  std::pmr::monotonic_buffer_resource resource_;
  std::pmr::string ss_{&resource_};
#endif
#else
  std::string ss_;
#endif
};

#define TO_STR(s) #s

#define GET_STRING(filename, line)                              \
  [] {                                                          \
    constexpr auto path = refvalue::meta_string{filename};      \
    constexpr size_t pos =                                      \
        path.rfind(std::filesystem::path::preferred_separator); \
    constexpr auto name = path.substr<pos + 1>();               \
    constexpr auto prefix = name + ":" + TO_STR(line);          \
    return "[" + prefix + "] ";                                 \
  }()

}  // namespace easylog