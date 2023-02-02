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
#include <filesystem>

#include "doctest.h"
#include "logging/easylog.h"

using namespace easylog;

std::string get_last_line(const std::string& filename) {
  std::string last_line;
  std::string temp;
  std::ifstream file(filename);
  if (file) {
    while (std::getline(file, temp)) {
      last_line = std::move(temp);
    }
  }

  return last_line;
}

TEST_CASE("test basic") {
  std::string filename = "easylog.txt";
  std::filesystem::remove(filename);
  easylog::init_log(Severity::DEBUG, filename, true, 5000, 1, true);

  ELOGV(INFO, "test");
  ELOGV(INFO, "it is a long string test %d %s", 2, "ok");
  CHECK(get_last_line(filename).rfind("test 2 ok") != std::string::npos);

  int len = 42;
  ELOGV(INFO, "rpc header data_len: %d, buf sz: %lu", len, 20);
  CHECK(get_last_line(filename).rfind("rpc header data_len: 42, buf sz: 20") !=
        std::string::npos);

  ELOG(INFO) << "test log";
  easylog::flush();
  CHECK(get_last_line(filename).rfind("test log") != std::string::npos);
  ELOG_INFO << "hello "
            << "easylog";
  CHECK(get_last_line(filename).rfind("hello easylog") != std::string::npos);
  ELOGI << "same";
  CHECK(get_last_line(filename).rfind("same") != std::string::npos);

  ELOG_DEBUG << "debug log";
  ELOGD << "debug log";
  CHECK(get_last_line(filename).rfind("debug log") != std::string::npos);
}
