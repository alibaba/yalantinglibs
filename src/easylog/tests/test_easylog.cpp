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
#include "doctest.h"
#include "logging/easylog.h"

using namespace easylog_ns;

std::string get_last_line(std::string_view filename) {
  std::string last_line;
  std::string temp;
  std::ifstream fin(filename);
  if (fin) {
    while (std::getline(fin, temp)) {
      last_line = std::move(temp);
    }
  }

  return last_line;
}

TEST_CASE("test basic") {
  std::string filename = "easylog.txt";
  easylog_ns::init_log(Severity::DEBUG, filename);
  ELOG(Severity::INFO) << "test log";
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
