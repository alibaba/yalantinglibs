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

std::string_view req_str =
    "R(GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg "
    "HTTP/1.1\r\n"
    "Content-Type: application/octet-stream"
    "Host: cinatra\r\n"
    "\r\n)";

std::string_view req_str1 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: cinatra\r\n"
    "\r\n)";

std::string_view req_str2 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "\r\n)";

std::string_view req_str3 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Sec-WebSocket-Extensions: permessage-deflate\r\n"
    "\r\n)";

std::string_view req_str4 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Content-Encoding: gzip\r\n"
    "\r\n)";

std::string_view req_str5 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Content-Encoding: deflate\r\n"
    "\r\n)";

std::string_view req_str6 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Content-Encoding: br\r\n"
    "\r\n)";

std::string_view req_str7 =
    "R(GET /ws "
    "HTTP/1.1\r\n"
    "Connection: upgrade\r\n"
    "Upgrade: websocket\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Content-Encoding: cinatra\r\n"
    "\r\n)";

TEST_CASE("http_request test") {
  http_parser parser{};
  int ret = parser.parse_request(req_str.data(), req_str.size(), 0);
  CHECK(ret);
  coro_http_request req(parser, nullptr);
  CHECK(parser.msg().empty());

  CHECK(req.get_accept_encoding().empty());
  CHECK(req.get_content_type() == content_type::octet_stream);
  CHECK(req.get_boundary().empty());

  req.set_aspect_data(std::string("test"));
  CHECK(req.get_aspect_data().size() == 1);
  req.set_aspect_data(std::vector<std::string>{"test", "aspect"});
  CHECK(req.get_aspect_data().size() == 2);
  CHECK(!req.is_support_compressed());
  CHECK(!req.is_upgrade());

  parser = {};
  parser.parse_request(req_str2.data(), req_str2.size(), 0);
  CHECK(!req.is_upgrade());

  parser = {};
  parser.parse_request(req_str3.data(), req_str3.size(), 0);
  CHECK(req.is_upgrade());
  CHECK(req.is_support_compressed());
  CHECK(req.get_encoding_type() == content_encoding::none);

  parser = {};
  parser.parse_request(req_str4.data(), req_str4.size(), 0);
  CHECK(req.is_upgrade());
  CHECK(req.get_encoding_type() == content_encoding::gzip);

  parser = {};
  parser.parse_request(req_str5.data(), req_str5.size(), 0);
  CHECK(req.is_upgrade());
  CHECK(req.get_encoding_type() == content_encoding::deflate);

  parser = {};
  parser.parse_request(req_str6.data(), req_str6.size(), 0);
  CHECK(req.is_upgrade());
  CHECK(req.get_encoding_type() == content_encoding::br);

  parser = {};
  parser.parse_request(req_str7.data(), req_str7.size(), 0);
  CHECK(req.is_upgrade());
  CHECK(req.get_encoding_type() == content_encoding::none);
}

TEST_CASE("uri test") {
  std::string uri = "https://example.com?name=tom";
  uri_t u;
  bool r = u.parse_from(uri.data());
  CHECK(r);
  CHECK(u.get_port() == "443");
  context c{u, http_method::GET};
  context c1{u, http_method::GET, "test"};
  CHECK(u.get_query() == "name=tom");

  uri = "https://example.com:521?name=tom";
  r = u.parse_from(uri.data());
  CHECK(r);
  CHECK(u.get_port() == "521");

  uri = "#https://example.com?name=tom";
  r = u.parse_from(uri.data());
  CHECK(!r);

  uri = "https##://example.com?name=tom";
  r = u.parse_from(uri.data());
  CHECK(!r);

  uri = "https://^example.com?name=tom";
  r = u.parse_from(uri.data());
  CHECK(!r);

  uri = "https://example.com?^name=tom";
  r = u.parse_from(uri.data());
  CHECK(!r);

  uri = "http://username:password@example.com";
  r = u.parse_from(uri.data());
  CHECK(r);
  CHECK(u.uinfo == "username:password");

  uri = "http://example.com/data.csv#row=4";
  r = u.parse_from(uri.data());
  CHECK(r);
  CHECK(u.fragment == "row=4");

  uri = "https://example.com?name=tom$";
  r = u.parse_from(uri.data());
  CHECK(r);

  uri = "https://example.com?name=tom!";
  r = u.parse_from(uri.data());
  CHECK(r);
}

TEST_CASE("client supplied session ids cannot create sessions") {
  auto &manager = session_manager::get();
  const std::string client_session_id = "client-chosen-session-id";
  REQUIRE_FALSE(manager.check_session_existence(client_session_id));

  std::string raw_request = "GET / HTTP/1.1\r\nCookie: " + CSESSIONID + "=" +
                            client_session_id + "\r\n\r\n";
  http_parser parser;
  REQUIRE(parser.parse_request(raw_request.data(), raw_request.size(), 0) > 0);
  coro_http_request request(parser, nullptr);

  CHECK(manager.get_session(client_session_id) == nullptr);
  CHECK(request.get_session(false) == nullptr);
  CHECK_FALSE(manager.check_session_existence(client_session_id));

  auto created_session = request.get_session();
  REQUIRE(created_session != nullptr);
  std::string generated_session_id = created_session->get_session_id();
  CHECK(generated_session_id != client_session_id);
  CHECK(generated_session_id.size() == 32);
  CHECK(generated_session_id.find_first_not_of("0123456789abcdef") ==
        std::string::npos);
  CHECK_FALSE(manager.check_session_existence(client_session_id));
  CHECK(manager.find_session(generated_session_id) == created_session);
  CHECK(manager.get_session(generated_session_id) == created_session);
  CHECK(request.get_session() == created_session);

  std::string existing_request = "GET / HTTP/1.1\r\nCookie: " + CSESSIONID +
                                 "=" + generated_session_id + "\r\n\r\n";
  http_parser existing_parser;
  REQUIRE(existing_parser.parse_request(existing_request.data(),
                                        existing_request.size(), 0) > 0);
  coro_http_request existing(existing_parser, nullptr);
  CHECK(existing.get_session(false) == created_session);

  std::string oversized_session_id(CINATRA_MAX_SESSION_ID_SIZE + 1, 'a');
  std::string oversized_request = "GET / HTTP/1.1\r\nCookie: " + CSESSIONID +
                                  "=" + oversized_session_id + "\r\n\r\n";
  http_parser oversized_parser;
  REQUIRE(oversized_parser.parse_request(oversized_request.data(),
                                         oversized_request.size(), 0) > 0);
  coro_http_request oversized(oversized_parser, nullptr);
  CHECK(oversized.get_session(false) == nullptr);
  CHECK_FALSE(manager.check_session_existence(oversized_session_id));

  auto replacement_session = oversized.get_session();
  REQUIRE(replacement_session != nullptr);
  CHECK(replacement_session->get_session_id() != oversized_session_id);

  created_session->invalidate();
  replacement_session->invalidate();
  manager.remove_expire_session();
  CHECK_FALSE(manager.check_session_existence(generated_session_id));
}
