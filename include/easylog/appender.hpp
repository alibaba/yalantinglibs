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
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

#include "record.hpp"
#include "util/concurrentqueue.h"

namespace easylog {
constexpr inline std::string_view BOM_STR = "\xEF\xBB\xBF";
struct QueueTraits : public moodycamel::ConcurrentQueueDefaultTraits {
  static const size_t BLOCK_SIZE = 256;
};

class appender {
 public:
  appender(const std::string &filename, bool async, bool enable_console,
           size_t max_file_size, size_t max_files, bool flush_every_time)
      : flush_every_time_(flush_every_time),
        enable_console_(enable_console),
        max_file_size_(max_file_size) {
    filename_ = filename;
    max_files_ = (std::min)(max_files, static_cast<size_t>(1000));
    open_log_file();
    if (async) {
      write_thd_ = std::thread([this] {
        while (!stop_) {
          if (max_files_ > 0 && file_size_ > max_file_size_ &&
              static_cast<size_t>(-1) != file_size_) {
            roll_log_files();
          }

          record_t record;
          if (queue_.try_dequeue(record)) {
            write_record(record);
          }

          if (queue_.size_approx() == 0) {
            std::unique_lock<std::mutex> lock(mtx_);
            cnd_.wait(lock, [&]() {
              return queue_.size_approx() > 0 || stop_;
            });
          }

          if (stop_) {
            if (queue_.size_approx() > 0) {
              while (queue_.try_dequeue(record)) {
                write_record(record);
              }
            }
          }
        }
      });
    }
  }

  void write_record(record_t &record) {
    char buf[32];
    size_t len = get_time_str(buf, record.get_time_point());

    write_str({buf, len});
    write_str({" ", 1});
    write_str(severity_str(record.get_severity()));
    write_str({" ", 1});

    auto [ptr, ec] = std::to_chars(buf, buf + 32, record.get_tid());

    write_str({"[", 1});
    write_str(std::string_view(buf, ptr - buf));
    write_str({"] ", 2});
    write_str(record.get_file_str());
    write_str(record.get_message());
    write_str({"\n", 1});

    if (enable_console_) {
      std::cout << std::flush;
    }
  }

  void write(record_t &&r) {
    queue_.enqueue(std::move(r));
    cnd_.notify_all();
  }

  template <typename String>
  void write(const String &str) {
    std::lock_guard guard(mtx_);
    if (max_files_ > 0 && file_size_ > max_file_size_ &&
        static_cast<size_t>(-1) != file_size_) {
      roll_log_files();
    }

    write_str(str);
  }

  void flush() {
    std::lock_guard guard(mtx_);
    file_.flush();
    file_.sync_with_stdio();
  }

  void stop() {
    std::lock_guard guard(mtx_);
    if (!write_thd_.joinable()) {
      return;
    }

    if (stop_) {
      return;
    }
    stop_ = true;
    cnd_.notify_one();
  }

  ~appender() {
    stop();
    flush();
    if (write_thd_.joinable())
      write_thd_.join();
  }

  template <size_t N>
  size_t get_time_str(char (&buf)[N], const auto &now) {
    const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now.time_since_epoch())
                           .count() %
                       1000;

    size_t endpos =
        std::strftime(buf, N, "%Y-%m-%d %H:%M:%S", std::localtime(&nowAsTimeT));
    snprintf(buf + endpos, N - endpos, ".%03d", (int)nowMs);
    return endpos + 4;
  }

 private:
  void open_log_file() {
    std::string filename = build_filename();
    file_.open(filename, std::ios::binary | std::ios::out | std::ios::app);
    if (file_) {
      std::error_code ec;
      size_t file_size = std::filesystem::file_size(filename, ec);
      if (ec) {
        std::cout << "get file size error" << std::flush;
        abort();
      }

      if (file_size == 0) {
        if (file_.write(BOM_STR.data(), BOM_STR.size())) {
          file_size_ += BOM_STR.size();
          is_first_write_ = false;
        }
      }
    }
  }

  std::string build_filename(int file_number = 0) {
    if (file_number == 0) {
      return filename_;
    }

    auto file_path = std::filesystem::path(filename_);
    std::string filename = file_path.stem().string();

    if (file_number > 0) {
      char buf[32];
      auto [ptr, ec] = std::to_chars(buf, buf + 32, file_number);
      filename.append(".").append(std::string_view(buf, ptr - buf));
    }

    if (file_path.has_extension()) {
      filename.append(file_path.extension().string());
    }

    return filename;
  }

  void roll_log_files() {
    file_.close();
    std::string last_filename = build_filename(max_files_ - 1);

    std::error_code ec;
    std::filesystem::remove(last_filename, ec);

    for (int file_number = max_files_ - 2; file_number >= 0; --file_number) {
      std::string current_fileName = build_filename(file_number);
      std::string next_fileName = build_filename(file_number + 1);

      std::filesystem::rename(current_fileName, next_fileName, ec);
    }

    open_log_file();
    is_first_write_ = false;
  }

  void write_str(std::string_view str) {
    if (file_.write(str.data(), str.size())) {
      if (flush_every_time_) {
        file_.flush();
      }

      file_size_ += str.size();
    }

    if (enable_console_) {
      std::cout << str;
    }
  }

  std::string filename_;
  bool enable_console_ = false;

  bool flush_every_time_;
  size_t file_size_ = 0;
  size_t max_file_size_ = 0;
  size_t max_files_ = 0;
  bool is_first_write_ = true;

  std::mutex mtx_;
  std::ofstream file_;

  moodycamel::ConcurrentQueue<record_t, QueueTraits> queue_;
  std::thread write_thd_;
  std::condition_variable cnd_;
  std::atomic<bool> stop_ = false;
};
}  // namespace easylog
