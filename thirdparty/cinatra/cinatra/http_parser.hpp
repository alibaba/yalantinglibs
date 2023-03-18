#pragma once
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "picohttpparser.h"

using namespace std::string_view_literals;

namespace cinatra {
class http_parser {
 public:
  int parse_response(const char *data, size_t size, int last_len) {
    int minor_version;

    num_headers_ = sizeof(headers_) / sizeof(headers_[0]);
    const char *msg;
    size_t msg_len;
    header_len_ =
        phr_parse_response(data, size, &minor_version, &status_, &msg, &msg_len,
                           headers_, &num_headers_, last_len);
    msg_ = {msg, msg_len};
    auto header_value = this->get_header_value("content-length");
    if (header_value.empty()) {
      body_len_ = 0;
    }
    else {
      body_len_ = atoi(header_value.data());
    }

    return header_len_;
  }

  std::string_view get_header_value(std::string_view key) const {
    for (size_t i = 0; i < num_headers_; i++) {
      if (iequal(headers_[i].name, headers_[i].name_len, key.data()))
        return std::string_view(headers_[i].value, headers_[i].value_len);
    }

    return {};
  }

  bool is_chunked() const {
    auto transfer_encoding = this->get_header_value("transfer-encoding");
    if (transfer_encoding == "chunked"sv) {
      return true;
    }

    return false;
  }

  bool is_ranges() const {
    auto transfer_encoding = this->get_header_value("Accept-Ranges");
    return !transfer_encoding.empty();
  }

  bool is_websocket() const {
    auto upgrade = this->get_header_value("Upgrade");
    return upgrade == "WebSocket"sv || upgrade == "websocket"sv;
  }

  bool keep_alive() const {
    if (is_websocket()) {
      return true;
    }
    auto val = this->get_header_value("connection");
    if (val.empty() || iequal(val.data(), val.length(), "keep-alive")) {
      return true;
    }

    return false;
  }

  int status() const { return status_; }

  int header_len() const { return header_len_; }

  int body_len() const { return body_len_; }

  int total_len() const { return header_len_ + body_len_; }

  bool is_location() {
    auto location = this->get_header_value("Location");
    return !location.empty();
  }

  std::string_view msg() const { return msg_; }

  std::pair<phr_header *, size_t> get_headers() {
    return {headers_, num_headers_};
  }

  void set_headers(
      const std::vector<std::pair<std::string, std::string>> &headers) {
    num_headers_ = headers.size();
    for (size_t i = 0; i < num_headers_; i++) {
      headers_[i].name = headers[i].first.data();
      headers_[i].name_len = headers[i].first.size();
      headers_[i].value = headers[i].second.data();
      headers_[i].value_len = headers[i].second.size();
    }
  }

 private:
  std::string_view get_header_value(phr_header *headers, size_t num_headers,
                                    std::string_view key) {
    for (size_t i = 0; i < num_headers; i++) {
      if (iequal(headers[i].name, headers[i].name_len, key.data()))
        return std::string_view(headers[i].value, headers[i].value_len);
    }

    return {};
  }

  bool iequal(const char *s, size_t l, const char *t) const {
    if (strlen(t) != l)
      return false;

    for (size_t i = 0; i < l; i++) {
      if (std::tolower(s[i]) != std::tolower(t[i]))
        return false;
    }

    return true;
  }

  int status_ = 0;
  std::string_view msg_;
  size_t num_headers_ = 0;
  int header_len_ = 0;
  int body_len_ = 0;
  struct phr_header headers_[100];
};
}  // namespace cinatra