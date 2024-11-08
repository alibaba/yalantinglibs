#include <string>

#include "doctest.h"
#include "ylt/coro_http/coro_http_client.hpp"
#include "ylt/coro_http/coro_http_server.hpp"

using namespace coro_http;

#define REQ                                                                   \
  "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg "         \
  "HTTP/1.1\r\n"                                                              \
  "Host: www.kittyhell.com\r\n"                                               \
  "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; "   \
  "rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "                                 \
  "Pathtraq/0.9\r\n"                                                          \
  "Accept: "                                                                  \
  "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"       \
  "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                              \
  "Accept-Encoding: gzip,deflate\r\n"                                         \
  "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                         \
  "Keep-Alive: 115\r\n"                                                       \
  "Connection: keep-alive\r\n"                                                \
  "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "       \
  "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "          \
  "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor." \
  "com|utmcct=/reader/|utmcmd=referral\r\n"                                   \
  "\r\n"

TEST_CASE("http parser test") {
  const char *method;
  size_t method_len;
  const char *path;
  size_t path_len;
  int minor_version;
  cinatra::http_header headers[64];
  size_t num_headers;
  int i, ret;
  bool has_connection, has_close, has_upgrade, has_query;

  num_headers = sizeof(headers) / sizeof(headers[0]);
  ret = cinatra::detail::phr_parse_request(
      REQ, sizeof(REQ) - 1, &method, &method_len, &path, &path_len,
      &minor_version, headers, &num_headers, 0, has_connection, has_close,
      has_upgrade, has_query);
  CHECK(ret == 703);
  CHECK(strncmp(method, "GET", method_len) == 0);
  CHECK(minor_version == 1);
  std::string name(headers[0].name);
  std::string value(headers[0].value);
  CHECK(name == "Host");
  CHECK(value == "www.kittyhell.com");
}
