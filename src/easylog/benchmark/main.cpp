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

#ifdef HAVE_GLOG
#include <glog/logging.h>
#endif

#include <filesystem>

#include "easylog/easylog.h"

using namespace easylog;

class ScopedTimer {
 public:
  ScopedTimer(const char *name)
      : m_name(name), m_beg(std::chrono::high_resolution_clock::now()) {}
  ScopedTimer(const char *name, uint64_t &ns) : ScopedTimer(name) {
    m_ns = &ns;
  }
  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_beg);
    if (m_ns)
      *m_ns = dur.count();

    std::cout << m_name << " : " << dur.count() << " ns\n";
  }

 private:
  const char *m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
  uint64_t *m_ns = nullptr;
};

void test_glog() {
#ifdef HAVE_GLOG
  std::filesystem::remove("glog.txt");
  FLAGS_log_dir = ".";
  FLAGS_minloglevel = google::GLOG_INFO;
  FLAGS_timestamp_in_logfile_name = false;
  google::SetLogDestination(google::INFO, "glog.txt");
  google::InitGoogleLogging("glog");

  {
    ScopedTimer timer("glog   ");
    for (int i = 0; i < 5000; i++)
      LOG(INFO) << "Hello, it is a long string test! " << 42 << 21 << 2.5;
  }
#endif
}

void test_easylog() {
  std::filesystem::remove("easylog.txt");
  easylog::init_log(Severity::DEBUG, "easylog.txt", false, 1024 * 1024, 1);
  {
    ScopedTimer timer("easylog");
    for (int i = 0; i < 5000; i++)
      ELOG(INFO) << "Hello, it is a long string test! " << 42 << 21 << 2.5;
  }
}

int main() {
  test_glog();
  test_easylog();
}
