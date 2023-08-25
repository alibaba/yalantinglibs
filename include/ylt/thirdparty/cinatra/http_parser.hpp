#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <span>
#include <string>
#include <string_view>

#include "cinatra_log_wrapper.hpp"
#include "picohttpparser.h"

using namespace std::string_view_literals;

#ifndef CINATRA_MAX_HTTP_HEADER_FIELD_SIZE
#define CINATRA_MAX_HTTP_HEADER_FIELD_SIZE 100
#endif

namespace cinatra {
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

  std::string_view get_header_value(std::string_view key) const {
    for (size_t i = 0; i < num_headers_; i++) {
      if (iequal(headers_[i].name, key))
        return headers_[i].value;
    }
    return {};
  }

  bool is_chunked() const {
    auto transfer_encoding = this->get_header_value("transfer-encoding"sv);
    if (transfer_encoding == "chunked"sv) {
      return true;
    }

    return false;
  }

  bool is_ranges() const {
    auto transfer_encoding = this->get_header_value("Accept-Ranges"sv);
    return !transfer_encoding.empty();
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
    if (val.empty() || iequal(val, "keep-alive"sv)) {
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

  std::span<http_header> get_headers() {
    return {headers_.data(), num_headers_};
  }

 private:
  bool iequal(std::string_view a, std::string_view b) const {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char a, char b) {
                        return tolower(a) == tolower(b);
                      });
  }

  int status_ = 0;
  std::string_view msg_;
  size_t num_headers_ = 0;
  int header_len_ = 0;
  int body_len_ = 0;
  std::array<http_header, CINATRA_MAX_HTTP_HEADER_FIELD_SIZE> headers_;
};
}  // namespace cinatra