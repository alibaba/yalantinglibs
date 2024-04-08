#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "cinatra_log_wrapper.hpp"
#include "define.h"
#include "picohttpparser.h"
#include "url_encode_decode.hpp"

using namespace std::string_view_literals;

#ifndef CINATRA_MAX_HTTP_HEADER_FIELD_SIZE
#define CINATRA_MAX_HTTP_HEADER_FIELD_SIZE 100
#endif

namespace cinatra {
inline bool iequal0(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
    return tolower(a) == tolower(b);
  });
}

class http_parser {
 public:
  int parse_response(const char *data, size_t size, int last_len) {
    int minor_version;

    num_headers_ = CINATRA_MAX_HTTP_HEADER_FIELD_SIZE;
    const char *msg;
    size_t msg_len;
    header_len_ = cinatra::detail::phr_parse_response(
        data, size, &minor_version, &status_, &msg, &msg_len, headers_.data(),
        &num_headers_, last_len);
    msg_ = {msg, msg_len};
    auto header_value = this->get_header_value("content-length"sv);
    if (header_value.empty()) {
      body_len_ = 0;
    }
    else {
      body_len_ = atoi(header_value.data());
    }
    if (header_len_ < 0) [[unlikely]] {
      CINATRA_LOG_WARNING << "parse http head failed";
      if (size == CINATRA_MAX_HTTP_HEADER_FIELD_SIZE) {
        CINATRA_LOG_ERROR << "the field of http head is out of max limit "
                          << CINATRA_MAX_HTTP_HEADER_FIELD_SIZE
                          << ", you can define macro "
                             "CINATRA_MAX_HTTP_HEADER_FIELD_SIZE to expand it.";
      }
    }
    return header_len_;
  }

  int parse_request(const char *data, size_t size, int last_len) {
    int minor_version;

    num_headers_ = CINATRA_MAX_HTTP_HEADER_FIELD_SIZE;

    const char *method;
    size_t method_len;
    const char *url;
    size_t url_len;

    bool has_query{};
    header_len_ = detail::phr_parse_request(
        data, size, &method, &method_len, &url, &url_len, &minor_version,
        headers_.data(), &num_headers_, last_len, has_connection_, has_close_,
        has_upgrade_, has_query);

    if (header_len_ < 0) [[unlikely]] {
      CINATRA_LOG_WARNING << "parse http head failed";
      if (size == CINATRA_MAX_HTTP_HEADER_FIELD_SIZE) {
        CINATRA_LOG_ERROR << "the field of http head is out of max limit "
                          << CINATRA_MAX_HTTP_HEADER_FIELD_SIZE
                          << ", you can define macro "
                             "CINATRA_MAX_HTTP_HEADER_FIELD_SIZE to expand it.";
      }
      return header_len_;
    }

    method_ = {method, method_len};
    url_ = {url, url_len};

    auto methd_type = method_type(method_);
    if (methd_type == http_method::GET || methd_type == http_method::HEAD) {
      body_len_ = 0;
    }
    else {
      auto content_len = this->get_header_value("content-length"sv);
      if (content_len.empty()) {
        body_len_ = 0;
      }
      else {
        body_len_ = atoi(content_len.data());
      }
    }

    full_url_ = url_;
    if (has_query) {
      size_t pos = url_.find('?');
      parse_query(url_.substr(pos + 1, url_len - pos - 1));
      url_ = {url, pos};
    }

    return header_len_;
  }

  bool has_connection() { return has_connection_; }

  bool has_close() { return has_close_; }

  bool has_upgrade() { return has_upgrade_; }

  std::string_view get_header_value(std::string_view key) const {
    for (size_t i = 0; i < num_headers_; i++) {
      if (iequal0(headers_[i].name, key))
        return headers_[i].value;
    }
    return {};
  }

  const auto &queries() const { return queries_; }

  std::string_view full_url() { return full_url_; }

  std::string_view get_query_value(std::string_view key) {
    if (auto it = queries_.find(key); it != queries_.end()) {
      return it->second;
    }
    else {
      return "";
    }
  }

  bool is_chunked() const {
    auto transfer_encoding = this->get_header_value("transfer-encoding"sv);
    if (transfer_encoding == "chunked"sv) {
      return true;
    }

    return false;
  }

  bool is_multipart() {
    auto content_type = get_header_value("Content-Type");
    if (content_type.empty()) {
      return false;
    }

    if (content_type.find("multipart") == std::string_view::npos) {
      return false;
    }

    return true;
  }

  std::string_view get_boundary() {
    auto content_type = get_header_value("Content-Type");
    size_t pos = content_type.find("=--");
    if (pos == std::string_view::npos) {
      return "";
    }

    return content_type.substr(pos + 1);
  }

  bool is_req_ranges() const {
    auto value = this->get_header_value("Range"sv);
    return !value.empty();
  }

  bool is_resp_ranges() const {
    auto value = this->get_header_value("Accept-Ranges"sv);
    return !value.empty();
  }

  bool is_websocket() const {
    auto upgrade = this->get_header_value("Upgrade"sv);
    return upgrade == "WebSocket"sv || upgrade == "websocket"sv;
  }

  bool keep_alive() const {
    if (is_websocket()) {
      return true;
    }
    auto val = this->get_header_value("connection"sv);
    if (val.empty() || iequal0(val, "keep-alive"sv)) {
      return true;
    }

    return false;
  }

  int status() const { return status_; }

  int header_len() const { return header_len_; }

  int body_len() const { return body_len_; }

  int total_len() const { return header_len_ + body_len_; }

  bool is_location() {
    auto location = this->get_header_value("Location"sv);
    return !location.empty();
  }

  std::string_view msg() const { return msg_; }

  std::string_view method() const { return method_; }

  std::string_view url() const { return url_; }

  std::span<http_header> get_headers() {
    return {headers_.data(), num_headers_};
  }

  void parse_query(std::string_view str) {
    std::string_view key;
    std::string_view val;
    size_t pos = 0;
    size_t length = str.length();
    for (size_t i = 0; i < length; i++) {
      char c = str[i];
      if (c == '=') {
        key = {&str[pos], i - pos};
        key = trim(key);
        pos = i + 1;
      }
      else if (c == '&') {
        val = {&str[pos], i - pos};
        val = trim(val);
        queries_.emplace(key, val);

        pos = i + 1;
      }
    }

    if (pos == 0) {
      return;
    }

    if ((length - pos) > 0) {
      val = {&str[pos], length - pos};
      val = trim(val);
      queries_.emplace(key, val);
    }
    else if ((length - pos) == 0) {
      queries_.emplace(key, "");
    }
  }

  std::string_view trim(std::string_view v) {
    v.remove_prefix((std::min)(v.find_first_not_of(" "), v.size()));
    v.remove_suffix(
        (std::min)(v.size() - v.find_last_not_of(" ") - 1, v.size()));
    return v;
  }

 private:
  int status_ = 0;
  std::string_view msg_;
  size_t num_headers_ = 0;
  int header_len_ = 0;
  int body_len_ = 0;
  bool has_connection_{};
  bool has_close_{};
  bool has_upgrade_{};
  std::array<http_header, CINATRA_MAX_HTTP_HEADER_FIELD_SIZE> headers_;
  std::string_view method_;
  std::string_view url_;
  std::string_view full_url_;
  std::unordered_map<std::string_view, std::string_view> queries_;
};
}  // namespace cinatra