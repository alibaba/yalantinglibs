#pragma once

#include <any>
#include <charconv>
#include <optional>
#include <regex>

#include "async_simple/coro/Lazy.h"
#include "define.h"
#include "http_parser.hpp"
#include "session.hpp"
#include "session_manager.hpp"
#include "utils.hpp"
#include "ws_define.h"

namespace cinatra {

inline std::vector<std::pair<int, int>> parse_ranges(std::string_view range_str,
                                                     size_t file_size,
                                                     bool &is_valid) {
  range_str = trim_sv(range_str);
  if (range_str.empty()) {
    return {{0, file_size - 1}};
  }

  if (range_str.find("--") != std::string_view::npos) {
    is_valid = false;
    return {};
  }

  if (range_str == "-") {
    return {{0, file_size - 1}};
  }

  std::vector<std::pair<int, int>> vec;
  auto ranges = split_sv(range_str, ",");
  for (auto range : ranges) {
    auto sub_range = split_sv(range, "-");
    auto fist_range = trim_sv(sub_range[0]);

    int start = 0;
    if (fist_range.empty()) {
      start = -1;
    }
    else {
      auto [ptr, ec] = std::from_chars(
          fist_range.data(), fist_range.data() + fist_range.size(), start);
      if (ec != std::errc{}) {
        is_valid = false;
        return {};
      }
    }

    int end = 0;
    if (sub_range.size() == 1) {
      end = file_size - 1;
    }
    else {
      auto second_range = trim_sv(sub_range[1]);
      if (second_range.empty()) {
        end = file_size - 1;
      }
      else {
        auto [ptr, ec] =
            std::from_chars(second_range.data(),
                            second_range.data() + second_range.size(), end);
        if (ec != std::errc{}) {
          is_valid = false;
          return {};
        }
      }
    }

    if (start > 0 && (start >= file_size || start == end)) {
      // out of range
      is_valid = false;
      return {};
    }

    if (end > 0 && end >= file_size) {
      end = file_size - 1;
    }

    if (start == -1) {
      start = file_size - end;
      end = file_size - 1;
    }

    vec.push_back({start, end});
  }
  return vec;
}

class coro_http_connection;
class coro_http_request {
 public:
  coro_http_request(http_parser &parser, coro_http_connection *conn)
      : parser_(parser), conn_(conn) {}

  std::string_view get_header_value(std::string_view key) {
    auto headers = parser_.get_headers();
    for (auto &header : headers) {
      if (iequal0(header.name, key)) {
        return header.value;
      }
    }

    return {};
  }

  std::string_view get_query_value(std::string_view key) {
    return parser_.get_query_value(key);
  }

  std::string get_decode_query_value(std::string_view key) {
    auto value = parser_.get_query_value(key);
    if (value.empty()) {
      return "";
    }

    return code_utils::get_string_by_urldecode(value);
  }

  std::span<http_header> get_headers() const { return parser_.get_headers(); }

  const auto &get_queries() const { return parser_.queries(); }

  void set_body(std::string &body) {
    body_ = body;
    auto type = get_content_type();
    if (type == content_type::urlencoded) {
      parser_.parse_query(body_);
    }
  }

  std::string_view get_body() const { return body_; }

  bool is_chunked() { return parser_.is_chunked(); }

  bool is_resp_ranges() { return parser_.is_resp_ranges(); }

  bool is_req_ranges() { return parser_.is_req_ranges(); }

  content_type get_content_type() {
    if (is_chunked())
      return content_type::chunked;

    auto content_type = get_header_value("content-type");
    if (!content_type.empty()) {
      if (content_type.find("application/x-www-form-urlencoded") !=
          std::string_view::npos) {
        return content_type::urlencoded;
      }
      else if (content_type.find("multipart/form-data") !=
               std::string_view::npos) {
        return content_type::multipart;
      }
      else if (content_type.find("application/octet-stream") !=
               std::string_view::npos) {
        return content_type::octet_stream;
      }
      else {
        return content_type::string;
      }
    }

    if (is_websocket_) {
      return content_type::websocket;
    }

    return content_type::unknown;
  }

  std::string_view get_url() { return parser_.url(); }

  std::string_view get_method() { return parser_.method(); }

  std::string_view get_boundary() {
    auto content_type = get_header_value("content-type");
    if (content_type.empty()) {
      return {};
    }
    return content_type.substr(content_type.rfind("=") + 1);
  }

  coro_http_connection *get_conn() { return conn_; }

  bool is_upgrade() {
    auto h = get_header_value("Connection");
    if (h.empty())
      return false;

    auto u = get_header_value("Upgrade");
    if (u.empty())
      return false;

    if (h != UPGRADE)
      return false;

    if (u != WEBSOCKET)
      return false;

    auto sec_ws_key = get_header_value("sec-websocket-key");
    if (sec_ws_key.empty() || sec_ws_key.size() != 24)
      return false;

    is_websocket_ = true;
    return true;
  }

  void set_aspect_data(const std::string &&key, const std::any &data) {
    aspect_data_[key] = data;
  }

  template <typename T>
  std::optional<T> get_aspect_data(const std::string &&key) {
    auto it = aspect_data_.find(key);
    if (it == aspect_data_.end()) {
      return std::optional<T>{};
    }

    try {
      return std::any_cast<T>(it->second);  // throws
    } catch (const std::bad_any_cast &e) {
      return std::optional<T>{};
    }
  }

  std::unordered_map<std::string_view, std::string_view> get_cookies(
      std::string_view cookie_str) const {
    auto cookies = get_cookies_map(cookie_str);
    return cookies;
  }

  std::shared_ptr<session> get_session(bool create = true) {
    auto &session_manager = session_manager::get();

    auto cookies = get_cookies(get_header_value("Cookie"));
    std::string session_id;
    auto iter = cookies.find(CSESSIONID);
    if (iter == cookies.end() && !create) {
      return nullptr;
    }
    else if (iter == cookies.end()) {
      session_id = session_manager.generate_session_id();
    }
    else {
      session_id = iter->second;
    }

    cached_session_id_ = session_id;
    return session_manager.get_session(session_id);
  }

  std::string get_cached_session_id() {
    std::string temp_session_id = "";
    cached_session_id_.swap(temp_session_id);
    return temp_session_id;
  }

  bool has_session() { return !cached_session_id_.empty(); }

  std::unordered_map<std::string, std::string> params_;
  std::smatch matches_;

 private:
  http_parser &parser_;
  std::string_view body_;
  coro_http_connection *conn_;
  bool is_websocket_;
  std::unordered_map<std::string, std::any> aspect_data_;
  std::string cached_session_id_;
};
}  // namespace cinatra