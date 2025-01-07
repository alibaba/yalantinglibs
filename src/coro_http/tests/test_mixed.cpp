#include "doctest.h"
#include "ylt/coro_http/coro_http_client.hpp"
#include "ylt/coro_http/coro_http_server.hpp"

using namespace coro_http;

/*
two requests.
one request ok/failed with outbuf/no outbuf
*/
TEST_CASE("test mixed async_request") {
  coro_http_server server(2, 9900);
  server.set_http_handler<GET, POST>("/", [](request& req, response& resp) {
    resp.set_status_and_content(status_type::ok, "ok");
  });
  server.async_start();
  std::string uri = "http://127.0.0.1:9900/";
  std::string failed_uri = "http://127.0.0.1:9900/failed";

  // loop ok-failed
  auto lazy = [&]() -> async_simple::coro::Lazy<void> {
    coro_http_client client{};
    req_context<> ctx{};
    for (size_t i = 0; i < 6; i++) {
      auto result = co_await client.async_request(uri, http_method::GET, ctx);
      CHECK(result.status == 200);
      result = co_await client.async_request(failed_uri, http_method::GET, ctx);
      CHECK(result.status != 200);
    }
  };
  async_simple::coro::syncAwait(lazy());
}

/*
three requests.
one request ok/failed;
another request async_upload ok/failed;
the third request ok/failed;
*/
TEST_CASE("test mixed request and upload") {}