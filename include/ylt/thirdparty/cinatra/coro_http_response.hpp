#pragma once
#include <charconv>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "define.h"
#include "response_cv.hpp"
#include "time_util.hpp"

namespace cinatra {
struct resp_header {
  std::string key;
  std::string value;
};

struct resp_header_sv {
  std::string_view key;
  std::string_view value;
};

enum class format_type {
  normal,
  chunked,
};

class coro_http_connection;
class coro_http_response {
 public:
  coro_http_response(coro_http_connection* conn)
      : status_(status_type::not_implemented),
        fmt_type_(format_type::normal),
        delay_(false),
        conn_(conn) {
    head_.reserve(128);
  }

  void set_status(cinatra::status_type status) { status_ = status; }
  void set_content(std::string content) { content_ = std::move(content); }
  void set_status_and_content(status_type status, std::string content) {
    status_ = status;
    content_ = std::move(content);
  }
  void set_delay(bool r) { delay_ = r; }
  bool get_delay() const { return delay_; }
  void set_format_type(format_type type) { fmt_type_ = type; }

  void add_header(auto k, auto v) {
    resp_headers_.emplace_back(resp_header{std::move(k), std::move(v)});
  }

  void set_keepalive(bool r) { keepalive_ = r; }

  void to_buffers(std::vector<asio::const_buffer>& buffers) {
    build_resp_head();

    buffers.push_back(asio::buffer(to_rep_string(status_)));
    buffers.push_back(asio::buffer(head_));
    if (!content_.empty()) {
      if (fmt_type_ == format_type::chunked) {
        to_chunked_buffers(buffers, content_, true);
      }
      else {
        buffers.push_back(asio::buffer(content_));
      }
    }
  }

  std::string_view to_hex_string(size_t val) {
    static char buf[20];
    auto [ptr, ec] = std::to_chars(std::begin(buf), std::end(buf), val, 16);
    return std::string_view{buf, size_t(std::distance(buf, ptr))};
  }

  void to_chunked_buffers(std::vector<asio::const_buffer>& buffers,
                          std::string_view chunk_data, bool eof) {
    if (!chunk_data.empty()) {
      // convert bytes transferred count to a hex string.
      auto chunk_size = to_hex_string(chunk_data.size());

      // Construct chunk based on rfc2616 section 3.6.1
      buffers.push_back(asio::buffer(chunk_size));
      buffers.push_back(asio::buffer(crlf));
      buffers.push_back(asio::buffer(chunk_data));
      buffers.push_back(asio::buffer(crlf));
    }

    // append last-chunk
    if (eof) {
      buffers.push_back(asio::buffer(last_chunk));
      buffers.push_back(asio::buffer(crlf));
    }
  }

  void build_resp_head() {
    if (std::find_if(resp_headers_.begin(), resp_headers_.end(),
                     [](resp_header& header) {
                       return header.key == "Host";
                     }) == resp_headers_.end()) {
      resp_headers_sv_.emplace_back(resp_header_sv{"Host", "cinatra"});
    }

    if (status_ >= status_type::not_found) {
      content_.append(to_string(status_));
    }

    if (fmt_type_ == format_type::chunked) {
      resp_headers_sv_.emplace_back(
          resp_header_sv{"Transfer-Encoding", "chunked"});
    }
    else {
      if (!content_.empty()) {
        auto [ptr, ec] = std::to_chars(buf_, buf_ + 32, content_.size());
        resp_headers_sv_.emplace_back(
            resp_header_sv{"Content-Length",
                           std::string_view(buf_, std::distance(buf_, ptr))});
      }
      else {
        resp_headers_sv_.emplace_back(resp_header_sv{"Content-Length", "0"});
      }
    }

    resp_headers_sv_.emplace_back(resp_header_sv{"Date", get_gmt_time_str()});

    if (keepalive_.has_value()) {
      bool keepalive = keepalive_.value();
      resp_headers_sv_.emplace_back(
          resp_header_sv{"Connection", keepalive ? "keep-alive" : "close"});
    }

    append_head(resp_headers_);
    append_head(resp_headers_sv_);
    head_.append(CRCF);
  }

  coro_http_connection* get_conn() { return conn_; }

  void clear() {
    head_.clear();
    content_.clear();

    resp_headers_.clear();
    resp_headers_sv_.clear();
    keepalive_ = {};
    delay_ = false;
    status_ = status_type::init;
    fmt_type_ = format_type::normal;
  }

  void append_head(auto& headers) {
    for (auto& [k, v] : headers) {
      head_.append(k);
      head_.append(":");
      head_.append(v);
      head_.append(CRCF);
    }
  }

 private:
  status_type status_;
  format_type fmt_type_;
  std::string head_;
  std::string content_;
  std::optional<bool> keepalive_;
  bool delay_;
  char buf_[32];
  std::vector<resp_header> resp_headers_;
  std::vector<resp_header_sv> resp_headers_sv_;
  coro_http_connection* conn_;
};
}  // namespace cinatra