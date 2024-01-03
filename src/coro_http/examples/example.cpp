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
using namespace coro_http;

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
  auto data = co_await client.async_get(uri);
  std::cout << data.net_err.message() << "\n";
  std::cout << data.status << std::endl;
#endif
  co_return;
}

async_simple::coro::Lazy<void> test_websocket(
    coro_http::coro_http_client &client) {
  coro_http_server server(1, 8090);
  server.set_http_handler<cinatra::GET>(
      "/ws_echo",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        websocket_result result{};

        while (true) {
          result = co_await req.get_conn()->read_websocket();
          if (result.ec) {
            std::cout << "err msg: " << result.ec.message() << "\n";
            break;
          }

          if (result.type == ws_frame_type::WS_CLOSE_FRAME) {
            std::cout << "close reason: " << result.data << "\n";
            break;
          }

          std::cout << "get ws data from client, data content: " << result.data
                    << "\n";

          auto ec = co_await req.get_conn()->write_websocket(result.data);
          if (ec) {
            std::cout << "err msg: " << ec.message() << "\n";
            break;
          }
        }
      });
  server.async_start();

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
  bool r = co_await client.async_ws_connect("ws://127.0.0.1:8090/ws_echo");
  if (!r) {
    co_return;
  }

  auto result = co_await client.async_send_ws("hello websocket");
  std::cout << result.net_err << "\n";
  result = co_await client.async_send_ws("test again", /*need_mask = */ false);
  std::cout << result.net_err << "\n";
  result = co_await client.async_send_ws_close("ws close");
  std::cout << result.net_err << "\n";

  std::this_thread::sleep_for(
      300ms);  // wait for server deal with all messages, avoid server destruct
               // immediately.
}

bool create_file(std::string filename, size_t file_size) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    return false;
  }

  std::string str(file_size, 'A');
  file.write(str.data(), str.size());
  return true;
}

async_simple::coro::Lazy<void> chunked_upload_download(
    coro_http::coro_http_client &client) {
  coro_http_server server(1, 8090);
  server.set_http_handler<cinatra::PUT, cinatra::POST>(
      "/chunked",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        assert(req.get_content_type() == content_type::chunked);
        chunked_result result{};
        coro_io::coro_file file{};
        size_t total = 0;
        co_await file.async_open("test_file", coro_io::flags::create_write);
        while (true) {
          result = co_await req.get_conn()->read_chunked();
          if (result.ec) {
            co_return;
          }

          co_await file.async_write(result.data.data(), result.data.size());
          total += result.data.size();
          if (result.eof) {
            break;
          }
        }

        file.close();
        std::cout << "content size: " << fs::file_size("test_file") << "\n";
        resp.set_status_and_content(status_type::ok, "chunked ok");
      });

  server.set_http_handler<cinatra::GET, cinatra::POST>(
      "/write_chunked",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        resp.set_format_type(format_type::chunked);
        bool ok;
        if (ok = co_await resp.get_conn()->begin_chunked(); !ok) {
          co_return;
        }

        std::vector<std::string> vec{"hello", " world", " ok"};

        for (auto &str : vec) {
          if (ok = co_await resp.get_conn()->write_chunked(str); !ok) {
            co_return;
          }
        }

        ok = co_await resp.get_conn()->end_chunked();

        // resp.set_status_and_content(status_type::ok, "chunked ok");
        // co_await resp.get_conn()->reply();
      });

  server.async_start();

  std::string filename = "chunked.txt";
  create_file(filename, 1024);

  coro_io::coro_file file{};
  co_await file.async_open(filename, coro_io::flags::read_only);

  std::string buf;
  detail::resize(buf, 100);

  auto fn = [&file, &buf]() -> async_simple::coro::Lazy<read_result> {
    auto [ec, size] = co_await file.async_read(buf.data(), buf.size());
    co_return read_result{{buf.data(), size}, file.eof(), ec};
  };

  auto result = co_await client.async_upload_chunked(
      "http://127.0.0.1:8090/chunked"sv, http_method::POST, std::move(fn));
  assert(result.status == 200);

  result = co_await client.async_get("http://127.0.0.1:8090/write_chunked");
  assert(result.status == 200);
  assert(result.resp_body == "hello world ok");
}

async_simple::coro::Lazy<void> multipart_upload_files(
    coro_http::coro_http_client &client) {
  coro_http_server server(1, 8090);
  server.set_http_handler<cinatra::PUT, cinatra::POST>(
      "/multipart_upload",
      [](coro_http_request &req,
         coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        assert(req.get_content_type() == content_type::multipart);
        auto boundary = req.get_boundary();

        while (true) {
          auto part_head = co_await req.get_conn()->read_part_head();
          if (part_head.ec) {
            co_return;
          }

          std::cout << part_head.name << "\n";
          std::cout << part_head.filename << "\n";

          std::shared_ptr<coro_io::coro_file> file;
          std::string filename;
          if (!part_head.filename.empty()) {
            file = std::make_shared<coro_io::coro_file>();
            filename = std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());

            size_t pos = part_head.filename.rfind('.');
            if (pos != std::string::npos) {
              auto extent = part_head.filename.substr(pos);
              filename += extent;
            }

            std::cout << filename << "\n";
            co_await file->async_open(filename, coro_io::flags::create_write);
            if (!file->is_open()) {
              resp.set_status_and_content(status_type::internal_server_error,
                                          "file open failed");
              co_return;
            }
          }

          auto part_body = co_await req.get_conn()->read_part_body(boundary);
          if (part_body.ec) {
            co_return;
          }

          if (!filename.empty()) {
            auto ec = co_await file->async_write(part_body.data.data(),
                                                 part_body.data.size());
            if (ec) {
              co_return;
            }

            file->close();
            assert(fs::file_size(filename) == 1024);
          }
          else {
            std::cout << part_body.data << "\n";
          }

          if (part_body.eof) {
            break;
          }
        }

        resp.set_status_and_content(status_type::ok, "ok");
        co_return;
      });

  server.async_start();

  std::string filename = "test_1024.txt";
  create_file(filename, 1024);

  client.add_str_part("test", "test value");
  client.add_file_part("test file", filename);

  std::string uri = "http://127.0.0.1:8090/multipart_upload";
  auto result = co_await client.async_upload_multipart(uri);

  std::cout << result.status << "\n";
}

async_simple::coro::Lazy<void> ranges_download_files(
    coro_http::coro_http_client &client) {
  auto result = co_await client.async_download(
      "http://uniquegoodshiningmelody.neverssl.com/favicon.ico", "myfile",
      "1-10,11-16");
  std::cout << result.status << "\n";
  if (result.status == 206 && !result.resp_body.empty()) {
    assert(fs::file_size("myfile") == 16);
  }
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

  coro_http_client chunked_client{};
  async_simple::coro::syncAwait(chunked_upload_download(chunked_client));

  coro_http::coro_http_client upload_client{};
  upload_client.set_req_timeout(std::chrono::seconds(3));
  async_simple::coro::syncAwait(multipart_upload_files(upload_client));

  coro_http::coro_http_client ranges_download_client{};
  ranges_download_client.set_req_timeout(std::chrono::seconds(10));
  async_simple::coro::syncAwait(ranges_download_files(ranges_download_client));
}