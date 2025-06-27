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
#include <cstdint>
#include <memory>
#include "ylt/util/ff_ptr.hpp"
#include "doctest.h"
using namespace doctest;
static std::string err_string;
int initer=(ylt::util::regist_ff_ptr_error_handler([](ylt::util::ff_ptr_error_type error, void* raw_pointer){
    auto str = ylt::util::b_stacktrace_get_string();
    err_string = to_string(error) +'\n' + str;
    free(str);
  }),1);
int64_t hi;
template<typename T>
void read(ylt::util::ff_ptr<T> ptr) {
  hi=*ptr;
}
TEST_CASE("testing shared_ptr heap-use-after-free") {
  err_string = "";
  ylt::util::ff_ptr<int64_t> raw_ptr;
  {
    auto ptr = ylt::util::ff_make_shared<int64_t>(1);
    raw_ptr = ptr.get();
    read<int64_t>(raw_ptr);
    CHECK(err_string.empty());
  }
  read<int64_t>(raw_ptr);
  std::cout <<err_string<<std::endl;
  CHECK(err_string.find("read")!=std::string::npos);
  CHECK(err_string.find("heap use after free")!=std::string::npos);
} 
TEST_CASE("testing shared_ptr double-free") {
  err_string = "";
  int64_t hi;
  {
    auto ptr = ylt::util::ff_shared_ptr<int64_t>(&hi,[](int64_t*){});
    ylt::util::ff_shared_ptr<int64_t> ptr2(ptr.get(),[](int64_t*){});
  }
  std::cout <<err_string<<std::endl;
  CHECK(err_string.find("double free")!=std::string::npos);
} 
TEST_CASE("testing unique_ptr heap-use-after free") {
 err_string = "";
  ylt::util::ff_ptr<int64_t> raw_ptr;
  int64_t hi;
  {
    auto ptr = ylt::util::ff_unique_ptr<int64_t,decltype([](int64_t*){})>(&hi);
    raw_ptr = ptr.get();
    read<int64_t>(raw_ptr);
    CHECK(err_string.empty());
  }
  read<int64_t>(raw_ptr);
  std::cout <<err_string<<std::endl;
  CHECK(err_string.find("read")!=std::string::npos);
  CHECK(err_string.find("heap use after free")!=std::string::npos);
} 
TEST_CASE("testing unique_ptr heap-use-after free2") {
 err_string = "";
  ylt::util::ff_ptr<__int128> raw_ptr;
  __int128 hi;
  {
    auto ptr = ylt::util::ff_unique_ptr<__int128,decltype([](__int128*){})>(&hi);
    raw_ptr = ptr.get();
    read<__int128>(raw_ptr);
    CHECK(err_string.empty());
  }
  read<__int128>(raw_ptr);
  std::cout << err_string << std::endl;
  CHECK(err_string.find("read")!=std::string::npos);
  CHECK(err_string.find("heap use after free")!=std::string::npos);
} 
TEST_CASE("testing unique_ptr double-free") {
  err_string = "";
  int64_t hi;
  {
    auto ptr = ylt::util::ff_unique_ptr<int64_t,decltype([](int64_t*){})>(&hi);
    ylt::util::ff_unique_ptr<int64_t,decltype([](int64_t*){})> ptr2(ptr.get());
  }
  std::cout <<err_string<<std::endl;
  CHECK(err_string.find("double free")!=std::string::npos);
} 