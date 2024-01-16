#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "ylt/coro_http/coro_http_server.hpp"
#include "ylt/struct_json/json_reader.h"
#include "ylt/struct_json/json_writer.h"

using namespace coro_http;

struct login_info_t {
  std::string_view type;
  std::string_view content;
  std::vector<std::string> user_list;
};
REFLECTION(login_info_t, type, content, user_list);

using logout_info_t = login_info_t;

struct user_info_t {
  std::string_view type;
  std::string_view from;
  std::string_view content;
};
REFLECTION(user_info_t, type, from, content);

struct message_t {
  std::string type;
  std::string_view content;
};
REFLECTION(message_t, type, content);

async_simple::coro::Lazy<void> broadcast(auto &conn_map,
                                         std::string &resp_str) {
  for (auto &[conn_ptr, user_name] : conn_map) {
    auto conn = (coro_http_connection *)conn_ptr;
    auto ec = co_await conn->write_websocket(resp_str);
    if (ec) {
      std::cout << ec.message() << "\n";
      continue;
    }
  }
  resp_str.clear();
}

int main() {
  std::cout << fs::current_path() << "\n";
  coro_http::coro_http_server server(1, 9001);
  server.set_static_res_dir("", "");
  std::mutex mtx;
  std::unordered_map<intptr_t, std::string> conn_map;
  server.set_http_handler<cinatra::GET>(
      "/",
      [&](coro_http_request &req,
          coro_http_response &resp) -> async_simple::coro::Lazy<void> {
        websocket_result result{};

        std::unordered_map<intptr_t, std::string> map;
        std::string resp_str;
        while (true) {
          result = co_await req.get_conn()->read_websocket();
          if (result.ec) {
            {
              std::scoped_lock lock(mtx);
              conn_map.erase((intptr_t)req.get_conn());
            }
            break;
          }

          message_t msg{};
          if (result.type == ws_frame_type::WS_CLOSE_FRAME) {
            std::cout << "close frame\n";
            msg.type = "logout";
          }
          else {
            struct_json::from_json(msg, result.data);
          }

          if (msg.type == "user") {
            std::string from;
            {
              std::scoped_lock lock(mtx);
              from = conn_map.at((intptr_t)req.get_conn());
              map = conn_map;
            }

            user_info_t info{msg.type, from, msg.content};
            struct_json::to_json(info, resp_str);
          }
          else if (msg.type == "login" || msg.type == "logout") {
            std::string user_name;

            {
              std::scoped_lock lock(mtx);
              if (msg.type == "login") {
                user_name = std::string{msg.content};
                conn_map.emplace((intptr_t)req.get_conn(), user_name);
              }
              else {
                user_name = conn_map.at((intptr_t)req.get_conn());
                conn_map.erase((intptr_t)req.get_conn());
              }
              map = conn_map;
            }

            if (!map.empty()) {
              std::vector<std::string> user_list;
              std::transform(map.begin(), map.end(),
                             std::back_inserter(user_list), [](auto &kv) {
                               return kv.second;
                             });
              logout_info_t info{msg.type, user_name, std::move(user_list)};
              struct_json::to_json(info, resp_str);
            }
          }

          std::cout << result.data << "\n";

          co_await broadcast(map, resp_str);
          if (msg.type == "logout") {
            break;
          }
        }
      });
  server.sync_start();
}