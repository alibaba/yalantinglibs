#include <string>

#include "doctest.h"
#include "ylt/coro_http/coro_http_client.hpp"
#include "ylt/coro_http/coro_http_server.hpp"

using namespace coro_http;

std::string_view REQ =
    "R(GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg "
    "HTTP/1.1\r\n"
    "Host: www.kittyhell.com\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; "
    "rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "
    "Pathtraq/0.9\r\n"
    "Accept: "
    "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"
    "Accept-Encoding: gzip,deflate\r\n"
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"
    "Keep-Alive: 115\r\n"
    "Connection: keep-alive\r\n"
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor."
    "com|utmcct=/reader/|utmcmd=referral\r\n"
    "\r\n)";

std::string_view multipart_str =
    "R(POST / HTTP/1.1\r\n"
    "User-Agent: PostmanRuntime/7.39.0\r\n"
    "Accept: */*\r\n"
    "Cache-Control: no-cache\r\n"
    "Postman-Token: 33c25732-1648-42ed-a467-cc9f1eb1e961\r\n"
    "Host: purecpp.cn\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: multipart/form-data; "
    "boundary=--------------------------559980232503017651158362\r\n"
    "Cookie: CSESSIONID=87343c8a24f34e28be05efea55315aab\r\n"
    "\r\n"
    "----------------------------559980232503017651158362\r\n"
    "Content-Disposition: form-data; name=\"test\"\r\n"
    "tom\r\n"
    "----------------------------559980232503017651158362--\r\n";

std::string_view bad_multipart_str =
    "R(POST / HTTP/1.1\r\n"
    "User-Agent: PostmanRuntime/7.39.0\r\n"
    "Accept: */*\r\n"
    "Cache-Control: no-cache\r\n"
    "Postman-Token: 33c25732-1648-42ed-a467-cc9f1eb1e961\r\n"
    "Host: purecpp.cn\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: multipart/form-data; boundary=559980232503017651158362\r\n"
    "Cookie: CSESSIONID=87343c8a24f34e28be05efea55315aab\r\n"
    "\r\n"
    "559980232503017651158362\r\n"
    "Content-Disposition: form-data; name=\"test\"\r\n"
    "tom\r\n"
    "559980232503017651158362--\r\n";

std::string_view resp_str =
    "R(HTTP/1.1 400 Bad Request\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 20\r\n"
    "Host: cinatra\r\n"
    "\r\n\r\n"
    "the url is not right)";

TEST_CASE("http_parser test") {
  http_parser parser{};
  parser.parse_request(REQ.data(), REQ.size(), 0);
  CHECK(parser.body_len() == 0);
  CHECK(parser.body_len() + parser.header_len() == parser.total_len());
  CHECK(parser.has_connection());

  parser = {};
  std::string_view str(REQ.data(), 20);
  int ret = parser.parse_request(str.data(), str.size(), 0);
  CHECK(ret < 0);

  parser = {};
  ret = parser.parse_request(multipart_str.data(), multipart_str.size(), 0);
  CHECK(ret > 0);
  auto boundary = parser.get_boundary();
  CHECK(boundary == "--------------------------559980232503017651158362");

  parser = {};
  ret = parser.parse_request(bad_multipart_str.data(), bad_multipart_str.size(),
                             0);
  CHECK(ret > 0);
  auto bad_boundary = parser.get_boundary();
  CHECK(bad_boundary.empty());

  parser = {};
  std::string_view part_resp(resp_str.data(), 20);
  ret = parser.parse_response(part_resp.data(), part_resp.size(), 0);
  CHECK(ret < 0);
}
