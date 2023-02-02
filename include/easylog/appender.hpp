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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

#include "record.hpp"

namespace easylog {
constexpr inline std::string_view BOM_STR = "\xEF\xBB\xBF";

class appender {
 public:
  appender(const std::string &filename, size_t max_file_size, size_t max_files,
           bool flush_every_time)
      : flush_every_time_(flush_every_time), max_file_size_(max_file_size) {
    filename_ = filename;
    max_files_ = (std::min)(max_files, static_cast<size_t>(1000));
  }

  template <typename String>
  void write(const String &str) {
    std::lock_guard guard(mtx_);
    if (is_first_write_) {
      open_log_file();
      is_first_write_ = false;
    }
    else if (max_files_ > 0 && file_size_ > max_file_size_ &&
             static_cast<size_t>(-1) != file_size_) {
      roll_log_files();
    }

    // It can be improved witch cache.
    if (file_.write(str.data(), str.size())) {
      // It can be improved: flush with some interval .
      if (flush_every_time_) {
        file_.flush();
      }

      file_size_ += str.size();
    }
  }

  void flush() {
    std::lock_guard guard(mtx_);
    file_.flush();
  }

 private:
  void open_log_file() {
    std::string filename = build_filename();
    file_.open(filename, std::ios::binary | std::ios::app);
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

  std::string filename_;

  bool flush_every_time_;
  size_t file_size_ = 0;
  size_t max_file_size_ = 0;
  size_t max_files_ = 0;
  bool is_first_write_ = true;

  std::mutex mtx_;
  std::ofstream file_;
};
}  // namespace easylog
