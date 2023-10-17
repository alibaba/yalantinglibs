#pragma once
#include "async_simple/coro/Lazy.h"
#include "define.h"
#include "http_parser.hpp"

namespace cinatra {
class coro_http_connection;
class coro_http_request {
 public:
  coro_http_request(http_parser& parser, coro_http_connection* conn)
      : parser_(parser), conn_(conn) {}

  std::string_view get_header_value(std::string_view key) {
    auto headers = parser_.get_headers();
    for (auto& header : headers) {
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

  const auto& get_queries() const { return parser_.queries(); }

  void set_body(std::string& body) {
    body_ = body;
    auto type = get_content_type();
    if (type == content_type::urlencoded) {
      parser_.parse_query(body_);
    }
  }

  std::string_view get_body() const { return body_; }

  bool is_chunked() {
    static bool thread_local is_chunk = parser_.is_chunked();
    return is_chunk;
  }

  content_type get_content_type() {
    static content_type thread_local content_type = get_content_type_impl();
    return content_type;
  }

  content_type get_content_type_impl() {
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

    return content_type::unknown;
  }

  coro_http_connection* get_conn() { return conn_; }

 private:
  http_parser& parser_;
  std::string_view body_;
  coro_http_connection* conn_;
};
}  // namespace cinatra