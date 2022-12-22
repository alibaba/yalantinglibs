/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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

#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace coro_rpc {
struct easylog_options {
  spdlog::level::level_enum log_level = spdlog::level::debug;
  std::string app_log_name = "easylog";
  std::string log_dir;
  bool always_flush = false;
  int flush_interval = 3;
  int max_size = 5 * 1024 * 1024;
  int max_files = 5;
};

struct source_location {
  constexpr source_location(const char *file_name = __builtin_FILE(),
                            const char *function_name = __builtin_FUNCTION(),
                            unsigned int line = __builtin_LINE()) noexcept
      : file_name_(file_name), function_name_(function_name), line_(line) {}
  constexpr const char *file_name() const noexcept { return file_name_; }
  constexpr const char *function_name() const noexcept {
    return function_name_;
  }
  constexpr unsigned int line() const noexcept { return line_; }

 private:
  const char *file_name_;
  const char *function_name_;
  const unsigned int line_;
};

namespace easylog {
[[nodiscard]] inline constexpr auto get_log_source_location(
    const source_location &location) {
  return spdlog::source_loc{location.file_name(),
                            static_cast<std::int32_t>(location.line()),
                            location.function_name()};
}
namespace {
inline bool has_init_ = false;
inline bool always_flush_ = false;

inline std::vector<spdlog::sink_ptr> get_sinks(const easylog_options &options) {
  std::vector<spdlog::sink_ptr> sinks;

  if (options.log_dir.empty()) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(options.log_level);
    sinks.push_back(console_sink);
  }
  else {
#ifdef _WIN32
    int pid = _getpid();
#else
    pid_t pid = getpid();
#endif

    std::string filename = options.log_dir;
    std::string name = options.app_log_name;
    name.append("_").append(std::to_string(pid)).append(".log");
    filename.append(name);
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filename, options.max_size, options.max_files);
    sinks.push_back(file_sink);
  }

  auto err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
  err_sink->set_level(spdlog::level::err);
  sinks.push_back(err_sink);

  return sinks;
}

template <typename... Args>
inline void log(spdlog::level::level_enum level, source_location location,
                fmt::format_string<Args...> fmt, Args &&...args) {
  std::filesystem::path p{location.file_name()};

  switch (level) {
    case spdlog::level::trace:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::trace, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::debug:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::debug, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::info:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::info, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::warn:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::warn, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::err:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::err, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::critical:
      spdlog::default_logger_raw()->log(get_log_source_location(location),
                                        spdlog::level::critical, fmt,
                                        std::forward<Args>(args)...);
      break;
    case spdlog::level::off:
    case spdlog::level::n_levels:
      break;
  }

  if (always_flush_) {
    spdlog::default_logger()->flush();
  }

  if (level == spdlog::level::critical) {
    spdlog::default_logger()->flush();
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace
inline void init_log(easylog_options options = {}, bool over_write = false) {
  if (has_init_ && !over_write) {
    return;
  }

  auto sinks = get_sinks(options);

  auto logger = std::make_shared<spdlog::logger>(options.app_log_name,
                                                 sinks.begin(), sinks.end());

  logger->set_level(options.log_level);
  spdlog::set_level(options.log_level);
  spdlog::set_default_logger(logger);

  always_flush_ = options.always_flush;
  if (!options.always_flush && options.flush_interval > 0) {
    spdlog::flush_every(std::chrono::seconds(options.flush_interval));
  }

  has_init_ = true;
}

inline void enable_always_flush(bool always_flush) {
  always_flush_ = always_flush;
}

template <typename... Args>
struct trace {
  constexpr trace(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {}) {
    log(spdlog::level::trace, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
trace(fmt::format_string<Args...> fmt, Args &&...args) -> trace<Args...>;

template <typename... Args>
struct debug {
  constexpr debug(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {}) {
    log(spdlog::level::debug, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
debug(fmt::format_string<Args...> fmt, Args &&...args) -> debug<Args...>;

template <typename... Args>
struct info {
  constexpr info(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {}) {
    log(spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }

  constexpr info(source_location location, fmt::format_string<Args...> fmt,
                 Args &&...args) {
    log(spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
info(fmt::format_string<Args...> fmt, Args &&...args) -> info<Args...>;

template <typename... Args>
info(source_location location, fmt::format_string<Args...> fmt, Args &&...args)
    -> info<Args...>;

template <typename... Args>
struct warn {
  constexpr warn(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {}) {
    log(spdlog::level::warn, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
warn(fmt::format_string<Args...> fmt, Args &&...args) -> warn<Args...>;

template <typename... Args>
struct error {
  constexpr error(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {}) {
    log(spdlog::level::err, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
error(fmt::format_string<Args...> fmt, Args &&...args) -> error<Args...>;

template <typename... Args>
struct critical {
  constexpr critical(fmt::format_string<Args...> fmt, Args &&...args,
                     source_location location = {}) {
    log(spdlog::level::critical, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
critical(fmt::format_string<Args...> fmt, Args &&...args) -> critical<Args...>;
}  // namespace easylog

}  // namespace coro_rpc
