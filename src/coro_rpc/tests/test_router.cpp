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
#include <coro_rpc/coro_rpc/router_impl.hpp>
#include <string_view>
#include <system_error>
#include <util/utils.hpp>
#include <variant>

#include "coro_rpc/coro_rpc/default_config/coro_rpc_config.hpp"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "doctest.h"
#include "struct_pack/struct_pack.hpp"
using namespace coro_rpc;

coro_rpc::internal::router<
    coro_rpc::config::coro_rpc_default_config::rpc_protocol>
    router;

template <typename T>
struct rpc_return_type {
  using type = T;
};
template <>
struct rpc_return_type<void> {
  using type = std::monostate;
};
template <typename T>
using rpc_return_type_t = typename rpc_return_type<T>::type;

namespace test_util {
template <typename T>
struct RPC_trait {
  using return_type = T;
};
template <>
struct RPC_trait<void> {
  using return_type = std::monostate;
};
template <auto func>
rpc_result<function_return_type_t<decltype(func)>> get_result(
    const auto &pair) {
  auto &&[rpc_errc, buffer] = pair;
  using T = function_return_type_t<decltype(func)>;
  using return_type = rpc_result<function_return_type_t<decltype(func)>>;
  rpc_return_type_t<T> ret;
  struct_pack::errc ec;
  rpc_error err;
  if (rpc_errc == std::errc{}) {
    ec = struct_pack::deserialize_to(ret, buffer);
    if (ec == struct_pack::errc::ok) {
      if constexpr (std::is_same_v<T, void>) {
        return {};
      }
      else {
        return std::move(ret);
      }
    }
  }
  else {
    err.code = rpc_errc;
    ec = struct_pack::deserialize_to(err.msg, buffer);
    if (ec == struct_pack::errc::ok) {
      return return_type{unexpect_t{}, std::move(err)};
    }
  }
  // deserialize failed.
  err = {std::errc::invalid_argument, "failed to deserialize rpc return value"};
  return return_type{unexpect_t{}, std::move(err)};
}

template <typename R>
void check_result(const auto &pair, size_t offset = 0) {
  auto [err, buffer] = pair;
  assert(err == err_ok);
  std::string_view data(buffer.data(), buffer.size());
  typename RPC_trait<R>::return_type r;
  auto res = struct_pack::deserialize_to(r, data);
  if (res != struct_pack::errc{}) {
    rpc_error r;
    auto res = struct_pack::deserialize_to(r, data);
    CHECK(res == struct_pack::errc{});
  }
}

auto pack() { return std::vector<char>{}; }

template <typename Arg>
auto pack(Arg &&arg) {
  return struct_pack::serialize(std::forward<Arg>(arg));
}

template <typename... Args>
auto pack(Args &&...args) {
  return struct_pack::serialize(std::make_tuple(std::forward<Args>(args)...));
}

size_t g_head_offset = 0;
size_t g_tail_offset = 0;

template <auto func, typename... Args>
auto test_route(auto conn, Args &&...args) {
  auto buf = pack(std::forward<Args>(args)...);
  constexpr auto id = func_id<func>();
  auto handler = router.get_handler(id);

  return router.route(
      id, handler,
      std::string_view{buf.data() + g_head_offset, buf.size() - g_tail_offset},
      conn, std::variant<coro_rpc::config::struct_pack_protocol>{});
}

template <auto func, typename... Args>
void test_route_and_check(auto conn, Args &&...args) {
  auto pair = test_route<func>(conn, std::forward<Args>(args)...);
  using R = function_return_type_t<decltype(func)>;
  check_result<R>(pair, RESP_HEAD_LEN);
}
}  // namespace test_util

void foo(int val) { std::cout << "foo " << val << "\n"; }
void foo1(coro_rpc::connection<void> conn, int val) {
  std::cout << "foo1 " << val << "\n";
}
void foo2(coro_rpc::connection<void> conn) {
  std::cout << "foo2 "
            << "\n";
}

void bar() {}

void bar3(int val) { std::cout << "bar3 val=" << val << "\n"; }

using namespace test_util;

std::shared_ptr<coro_connection> coro_conn = nullptr;

struct person {
  int id;
  std::string name;
};

person get_person(const person &p) {
  std::cout << "get_person: " << p.id << ", " << p.name << "\n";
  return p;
}

person get_person1(const person &p, int id, std::string name) {
  std::cout << "get_person1: " << p.id << ", " << p.name << ", " << id << ", "
            << name << "\n";
  return p;
}

void not_register_func(int) {}

async_simple::coro::Lazy<void> coro_func() {
  std::cout << "hello, it's in coro function\n";
  co_return;
}

TEST_CASE("testing coro_handler") {
  router.regist_handler<coro_func>();
  auto buf = pack();
  constexpr auto id = func_id<coro_func>();
  auto handler = router.get_coro_handler(id);

  async_simple::coro::syncAwait(router.route_coro(
      id, handler,
      std::string_view{buf.data() + g_head_offset, buf.size() - g_tail_offset},
      coro_conn, std::variant<coro_rpc::config::struct_pack_protocol>{}));
}

TEST_CASE("testing not registered func") {
  auto pair = test_route<not_register_func>(coro_conn, 42);
  CHECK(pair.first == std::errc::function_not_supported);
}

void plus_one(int val) {}
void plus_one1(std::shared_ptr<coro_connection> conn, int val) {}
std::string get_str(std::string str) { return str; }

static void plus_two(int a, int b) {}

class test_class {
 public:
  void plus_one(int val) {}
  std::string get_str(std::string str) { return str; }
  static void plus_two(int a, int b) {}
};

TEST_CASE("testing invalid arguments") {
  std::pair<std::errc, std::string> pair{};

  SUBCASE("test member functions") {
    test_class obj{};
    router.regist_handler<&test_class::plus_one, &test_class::get_str>(&obj);
    pair = test_route<plus_one>(coro_conn, 42);
    CHECK(pair.first == std::errc::function_not_supported);

    pair = test_route<&test_class::plus_one>(coro_conn, 42);
    CHECK(pair.first == err_ok);

    pair = test_route<&test_class::plus_one>(coro_conn);
    CHECK(pair.first == std::errc::invalid_argument);

    pair = test_route<&test_class::plus_one>(coro_conn, 42, 42);
    CHECK(pair.first == std::errc::invalid_argument);

    pair = test_route<&test_class::plus_one>(coro_conn, "test");
    CHECK(pair.first == std::errc::invalid_argument);

    pair = test_route<&test_class::get_str>(coro_conn, "test");
    CHECK(pair.first == std::errc::invalid_argument);

    pair = test_route<&test_class::get_str>(coro_conn, std::string("test"));
    CHECK(pair.first == err_ok);

    auto r = get_result<&test_class::get_str>(pair);
    CHECK(r.value() == "test");
  }

  router.regist_handler<plus_one>();
  router.regist_handler<get_str>();

  pair = test_route<get_str>(coro_conn, "test");
  CHECK(pair.first == std::errc::invalid_argument);

  pair = test_route<get_str>(coro_conn, std::string("test"));
  CHECK(pair.first == err_ok);
  auto r = get_result<get_str>(pair);
  CHECK(r.value() == "test");

  pair = test_route<plus_one>(coro_conn, 42, 42);
  CHECK(pair.first == std::errc::invalid_argument);

  pair = test_route<plus_one>(coro_conn);
  CHECK(pair.first == std::errc::invalid_argument);

  pair = test_route<plus_one>(coro_conn, 42);
  CHECK(pair.first == err_ok);

  pair = test_route<plus_one>(coro_conn, std::string("invalid arguments"));
  CHECK(pair.first == std::errc::invalid_argument);

  // register_handler<plus_one1>();
  // test_route<plus_one1>(coro_conn, 42); // will crash
}

TEST_CASE("testing invalid buffer") {
  std::pair<std::errc, std::string> pair{};
  pair = test_route<plus_one>(coro_conn, 42);
  CHECK(pair.first == err_ok);

  g_head_offset = 2;
  pair = test_route<plus_one>(coro_conn, 42);
  CHECK(pair.first == std::errc::invalid_argument);
  g_head_offset = 0;

  g_tail_offset = 2;
  pair = test_route<plus_one>(coro_conn, 42);
  CHECK(pair.first == std::errc::invalid_argument);
  g_tail_offset = 0;
}

void throw_exception_func() { throw std::runtime_error("runtime error"); }

void throw_exception_func1() { throw 1; }

TEST_CASE("testing exceptions") {
  router.regist_handler<throw_exception_func, throw_exception_func1>();

  std::pair<std::errc, std::string> pair{};
  pair = test_route<throw_exception_func>(coro_conn);
  CHECK(pair.first == std::errc::interrupted);
  auto r = get_result<throw_exception_func>(pair);
  std::cout << r.error().msg << "\n";

  pair = test_route<throw_exception_func1>(coro_conn);
  CHECK(pair.first == std::errc::interrupted);
  r = get_result<throw_exception_func>(pair);
  std::cout << r.error().msg << "\n";
}

TEST_CASE("testing object arguments") {
  router.regist_handler<get_person>();

  person p{1, "tom"};
  auto buf = struct_pack::serialize(p);
  person p1;
  auto ec = struct_pack::deserialize_to(p1, buf);
  REQUIRE(ec == struct_pack::errc{});
  test_route_and_check<get_person>(coro_conn, p);

  router.regist_handler<get_person1>();
  test_route_and_check<get_person1>(coro_conn, p, 42, std::string("jerry"));
}

TEST_CASE("testing basic rpc register and route") {
  router.regist_handler<bar>();
  router.regist_handler<bar3>();

  router.regist_handler<foo, foo1, foo2>();

  test_route_and_check<foo>(coro_conn, 42);
  test_route_and_check<foo1>(coro_conn, 42);
  test_route_and_check<foo2>(coro_conn);

  SUBCASE("test static functions") {
    router.regist_handler<plus_two>();
    router.regist_handler<test_class::plus_two>();

    test_route_and_check<plus_two>(coro_conn, 42, 42);
    test_route_and_check<test_class::plus_two>(coro_conn, 42, 42);
  }
}

using namespace coro_rpc;
using namespace coro_rpc::internal;
TEST_CASE("test get_return_type in connection") {
  SUBCASE("no conn") {
    auto ret = get_return_type<false, int>();
    CHECK(ret == 0);
  }
  SUBCASE("return void") {
    static_assert(
        std::is_same_v<decltype(get_return_type<true, connection<void>>()),
                       void>);
  }
  SUBCASE("return std::string") {
    static_assert(std::is_same_v<
                  decltype(get_return_type<true, connection<std::string>>()),
                  std::string>);
  }
}