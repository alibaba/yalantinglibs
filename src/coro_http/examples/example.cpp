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
#include <iostream>

#include "ylt/coro_http/coro_http_client.hpp"
#include "ylt/coro_http/coro_http_server.hpp"

using namespace std::chrono_literals;

void test_sync_client() {
  {
    std::string uri = "http://www.baidu.com";
    coro_http::coro_http_client client{};
    auto result = client.get(uri);
    if (result.net_err) {
      std::cout << result.net_err.message() << "\n";
    }
    std::cout << result.status << "\n";
    std::cout << result.resp_body << "\n";

    result = client.post(uri, "hello", coro_http::req_content_type::json);
    std::cout << result.status << "\n";
  }

  {
    coro_http::coro_http_client client{};
    std::string uri = "http://cn.bing.com";
    auto result = client.get(uri);
    if (result.net_err) {
      std::cout << result.net_err.message() << "\n";
    }
    std::cout << result.status << "\n";

    result = client.post(uri, "hello", coro_http::req_content_type::json);
    std::cout << result.status << "\n";
  }
}

async_simple::coro::Lazy<void> test_async_client(
    coro_http::coro_http_client &client) {
  std::string uri = "http://www.baidu.com";

  auto result = co_await client.connect(uri);
  if (result.net_err) {
    std::cout << result.net_err.message() << "\n";
  }
  std::cout << result.status << "\n";

  result = co_await client.async_get(uri);
  if (result.net_err) {
    std::cout << result.net_err.message() << "\n";
  }
  std::cout << result.status << "\n";

  result = co_await client.async_get(uri);
  std::cout << result.status << "\n";

  result = co_await client.async_post(uri, "hello",
                                      coro_http::req_content_type::string);
  std::cout << result.status << "\n";
}

async_simple::coro::Lazy<void> test_async_ssl_client(
    coro_http::coro_http_client &client) {
#ifdef CINATRA_ENABLE_SSL
  std::string uri = "https://cn.bing.com";
  [[maybe_unused]] auto ec = client.init_ssl("cn.bing.com");
  auto data = co_await client.async_get(uri);
  std::cout << data.net_err.message() << "\n";
  std::cout << data.status << std::endl;
#endif
  co_return;
}

async_simple::coro::Lazy<void> test_websocket(
    coro_http::coro_http_client &client) {
  client.on_ws_close([](std::string_view reason) {
    std::cout << "web socket close " << reason << std::endl;
  });
  client.on_ws_msg([](coro_http::resp_data data) {
    if (data.net_err) {
      std::cout << data.net_err.message() << "\n";
      return;
    }

    std::cout << data.resp_body << std::endl;
  });

  // connect to your websocket server.
  bool r = co_await client.async_ws_connect("ws://localhost:8090/ws");
  if (!r) {
    co_return;
  }

  auto result = co_await client.async_send_ws("hello websocket");
  std::cout << result.net_err << "\n";
  result = co_await client.async_send_ws("test again", /*need_mask = */ false);
  std::cout << result.net_err << "\n";
  result = co_await client.async_send_ws_close("ws close");
  std::cout << result.net_err << "\n";
}

async_simple::coro::Lazy<void> upload_files(
    coro_http::coro_http_client &client) {
  client.add_str_part("hello", "world");
  client.add_str_part("key", "value");
  client.add_file_part("test", "test.jpg");
  std::string uri = "http://yoururl.com";
  auto result = co_await client.async_upload_multipart(uri);
  std::cout << result.net_err << "\n";
  std::cout << result.status << "\n";

  result = co_await client.async_upload_multipart(uri, "test", "test.jpg");
  std::cout << result.status << "\n";
}

async_simple::coro::Lazy<void> download_files(
    coro_http::coro_http_client &client) {
  auto result = co_await client.async_download("http://example.com/test.jpg",
                                               "myfile.jpg");
  std::cout << result.status << "\n";
}

async_simple::coro::Lazy<void> ranges_download_files(
    coro_http::coro_http_client &client) {
  auto result = co_await client.async_download("http://example.com/test.txt",
                                               "myfile.txt", "1-10,11-16");
  std::cout << result.status << "\n";
}

void use_out_buf() {
  using namespace coro_http;
  std::string str;
  str.resize(10);
  std::string url = "http://cn.bing.com";

  str.resize(16400);
  coro_http_client client;
  auto ret = client.async_request(url, http_method::GET, req_context<>{}, {},
                                  std::span<char>{str.data(), str.size()});
  auto result = async_simple::coro::syncAwait(ret);
  bool ok = result.status == 200 || result.status == 301;
  assert(ok);
  std::string_view sv(str.data(), result.resp_body.size());
  assert(result.resp_body == sv);
}

void test_coro_http_server() {
  using namespace coro_http;
  coro_http::coro_http_server server(1, 9001);
  server.set_http_handler<coro_http::GET, coro_http::POST>(
      "/", [](coro_http_request &req, coro_http_response &resp) {
        // response in io thread.
        resp.set_status_and_content(coro_http::status_type::ok, "hello world");
      });

  server.set_http_handler<coro_http::GET, coro_http::POST>(
      "/coro",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        co_await coro_io::post([&]() {
          // coroutine in other thread.
          resp.set_status_and_content(coro_http::status_type::ok,
                                      "hello world in coro");
        });
      });

  server.set_http_handler<cinatra::GET>(
      "/ws_source",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        assert(req.get_content_type() == content_type::websocket);
        std::string out_str;
        websocket_result result{};
        while (!result.eof) {
          result = co_await req.get_conn()->read_websocket();
          if (result.ec) {
            break;
          }

          if (result.type == ws_frame_type::WS_CLOSE_FRAME) {
            std::cout << "close frame\n";
            break;
          }

          out_str.append(result.data);

          auto ec = co_await req.get_conn()->write_websocket(result.data);
          if (ec) {
            continue;
          }
        }

        std::cout << out_str << "\n";
      });

  server.async_start();
  std::this_thread::sleep_for(200ms);

  coro_http_client client{};
  resp_data result;
  result = client.get("http://127.0.0.1:9001/");
  assert(result.status == 200);
  assert(result.resp_body == "hello world");

  result = client.get("http://127.0.0.1:9001/coro");
  assert(result.status == 200);
  assert(result.resp_body == "hello world in coro");
}

int main() {
  test_coro_http_server();
  test_sync_client();
  use_out_buf();

  coro_http::coro_http_client client{};
  async_simple::coro::syncAwait(test_async_client(client));

  coro_http::coro_http_client ssl_client{};
  async_simple::coro::syncAwait(test_async_ssl_client(ssl_client));

  coro_http::coro_http_client ws_client{};
  async_simple::coro::syncAwait(test_websocket(ws_client));

  coro_http::coro_http_client upload_client{};
  upload_client.set_req_timeout(std::chrono::seconds(3));
  async_simple::coro::syncAwait(upload_files(upload_client));

  coro_http::coro_http_client download_client{};
  download_client.set_req_timeout(std::chrono::seconds(3));
  async_simple::coro::syncAwait(download_files(download_client));

  coro_http::coro_http_client ranges_download_client{};
  ranges_download_client.set_req_timeout(std::chrono::seconds(3));
  async_simple::coro::syncAwait(ranges_download_files(ranges_download_client));
}