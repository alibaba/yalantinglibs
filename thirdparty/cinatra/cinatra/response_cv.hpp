#pragma once
#include <string_view>

#include "define.h"

namespace cinatra {
enum class status_type {
  init,
  switching_protocols = 101,
  ok = 200,
  created = 201,
  accepted = 202,
  no_content = 204,
  partial_content = 206,
  multiple_choices = 300,
  moved_permanently = 301,
  moved_temporarily = 302,
  not_modified = 304,
  temporary_redirect = 307,
  bad_request = 400,
  unauthorized = 401,
  forbidden = 403,
  not_found = 404,
  conflict = 409,
  internal_server_error = 500,
  not_implemented = 501,
  bad_gateway = 502,
  service_unavailable = 503
};

enum class content_encoding { gzip, none };

inline std::string_view ok = "OK";
inline std::string_view created =
    "<html>"
    "<head><title>Created</title></head>"
    "<body><h1>201 Created</h1></body>"
    "</html>";

inline std::string_view accepted =
    "<html>"
    "<head><title>Accepted</title></head>"
    "<body><h1>202 Accepted</h1></body>"
    "</html>";

inline std::string_view no_content =
    "<html>"
    "<head><title>No Content</title></head>"
    "<body><h1>204 Content</h1></body>"
    "</html>";

inline std::string_view multiple_choices =
    "<html>"
    "<head><title>Multiple Choices</title></head>"
    "<body><h1>300 Multiple Choices</h1></body>"
    "</html>";

inline std::string_view moved_permanently =
    "<html>"
    "<head><title>Moved Permanently</title></head>"
    "<body><h1>301 Moved Permanently</h1></body>"
    "</html>";

inline std::string_view temporary_redirect =
    "<html>"
    "<head><title>Temporary Redirect</title></head>"
    "<body><h1>307 Temporary Redirect</h1></body>"
    "</html>";

inline std::string_view moved_temporarily =
    "<html>"
    "<head><title>Moved Temporarily</title></head>"
    "<body><h1>302 Moved Temporarily</h1></body>"
    "</html>";

inline std::string_view not_modified =
    "<html>"
    "<head><title>Not Modified</title></head>"
    "<body><h1>304 Not Modified</h1></body>"
    "</html>";

inline std::string_view bad_request =
    "<html>"
    "<head><title>Bad Request</title></head>"
    "<body><h1>400 Bad Request</h1></body>"
    "</html>";

inline std::string_view unauthorized =
    "<html>"
    "<head><title>Unauthorized</title></head>"
    "<body><h1>401 Unauthorized</h1></body>"
    "</html>";

inline std::string_view forbidden =
    "<html>"
    "<head><title>Forbidden</title></head>"
    "<body><h1>403 Forbidden</h1></body>"
    "</html>";

inline std::string_view not_found =
    "<html>"
    "<head><title>Not Found</title></head>"
    "<body><h1>404 Not Found</h1></body>"
    "</html>";

inline std::string_view conflict =
    "<html>"
    "<head><title>Conflict</title></head>"
    "<body><h1>409 Conflict</h1></body>"
    "</html>";

inline std::string_view internal_server_error =
    "<html>"
    "<head><title>Internal Server Error</title></head>"
    "<body><h1>500 Internal Server Error</h1></body>"
    "</html>";

inline std::string_view not_implemented =
    "<html>"
    "<head><title>Not Implemented</title></head>"
    "<body><h1>501 Not Implemented</h1></body>"
    "</html>";

inline std::string_view bad_gateway =
    "<html>"
    "<head><title>Bad Gateway</title></head>"
    "<body><h1>502 Bad Gateway</h1></body>"
    "</html>";

inline std::string_view service_unavailable =
    "<html>"
    "<head><title>Service Unavailable</title></head>"
    "<body><h1>503 Service Unavailable</h1></body>"
    "</html>";

inline constexpr std::string_view switching_protocols =
    "HTTP/1.1 101 Switching Protocals\r\n";
inline constexpr std::string_view rep_ok = "HTTP/1.1 200 OK\r\n";
inline constexpr std::string_view rep_created = "HTTP/1.1 201 Created\r\n";
inline constexpr std::string_view rep_accepted = "HTTP/1.1 202 Accepted\r\n";
inline constexpr std::string_view rep_no_content =
    "HTTP/1.1 204 No Content\r\n";
inline constexpr std::string_view rep_partial_content =
    "HTTP/1.1 206 Partial Content\r\n";
inline constexpr std::string_view rep_multiple_choices =
    "HTTP/1.1 300 Multiple Choices\r\n";
inline constexpr std::string_view rep_moved_permanently =
    "HTTP/1.1 301 Moved Permanently\r\n";
inline constexpr std::string_view rep_temporary_redirect =
    "HTTP/1.1 307 Temporary Redirect\r\n";
inline constexpr std::string_view rep_moved_temporarily =
    "HTTP/1.1 302 Moved Temporarily\r\n";
inline constexpr std::string_view rep_not_modified =
    "HTTP/1.1 304 Not Modified\r\n";
inline constexpr std::string_view rep_bad_request =
    "HTTP/1.1 400 Bad Request\r\n";
inline constexpr std::string_view rep_unauthorized =
    "HTTP/1.1 401 Unauthorized\r\n";
inline constexpr std::string_view rep_forbidden = "HTTP/1.1 403 Forbidden\r\n";
inline constexpr std::string_view rep_not_found = "HTTP/1.1 404 Not Found\r\n";
inline constexpr std::string_view rep_conflict = "HTTP/1.1 409 Conflict\r\n";
inline constexpr std::string_view rep_internal_server_error =
    "HTTP/1.1 500 Internal Server Error\r\n";
inline constexpr std::string_view rep_not_implemented =
    "HTTP/1.1 501 Not Implemented\r\n";
inline constexpr std::string_view rep_bad_gateway =
    "HTTP/1.1 502 Bad Gateway\r\n";
inline constexpr std::string_view rep_service_unavailable =
    "HTTP/1.1 503 Service Unavailable\r\n";

inline constexpr std::string_view rep_html =
    "Content-Type: text/html; charset=UTF-8\r\n";
inline constexpr std::string_view rep_json =
    "Content-Type: application/json; charset=UTF-8\r\n";
inline constexpr std::string_view rep_string =
    "Content-Type: text/plain; charset=UTF-8\r\n";
inline constexpr std::string_view rep_multipart =
    "Content-Type: multipart/form-data; boundary=";

inline constexpr std::string_view rep_keep = "Connection: keep-alive\r\n";
inline constexpr std::string_view rep_close = "Connection: close     \r\n";
inline constexpr std::string_view rep_len = "Content-Length: ";
inline constexpr std::string_view rep_crcf = "\r\n";
inline constexpr std::string_view rep_server = "Server: cinatra\r\n";

inline const char name_value_separator[] = {':', ' '};
// inline std::string_view crlf = "\r\n";

inline const char crlf[] = {'\r', '\n'};
inline const char last_chunk[] = {'0', '\r', '\n'};
inline const std::string http_chunk_header =
    "HTTP/1.1 200 OK\r\n"
    "Transfer-Encoding: chunked\r\n";
/*"Content-Type: video/mp4\r\n"
"\r\n";*/

inline const std::string http_range_chunk_header =
    "HTTP/1.1 206 Partial Content\r\n"
    "Transfer-Encoding: chunked\r\n";
/*"Content-Type: video/mp4\r\n"
"\r\n";*/

inline constexpr auto to_content_type_str(req_content_type type) {
  switch (type) {
    case req_content_type::html:
      return rep_html;
    case req_content_type::json:
      return rep_json;
    case req_content_type::string:
      return rep_string;
    case req_content_type::multipart:
      return rep_multipart;
    default:
      return ""sv;
  }
}

namespace detail {
template <unsigned... digits>
struct to_chars {
  static constexpr std::array<char, sizeof...(digits) + 18> value = {
      'C',
      'o',
      'n',
      't',
      'e',
      'n',
      't',
      '-',
      'L',
      'e',
      'n',
      'g',
      't',
      'h',
      ':',
      ' ',
      ('0' + digits)...,
      '\r',
      '\n'};
};

//        template<unsigned... digits>
//        const char to_chars<digits...>::value[] = { ('0' + digits)... , '\r',
//        '\n', 0 };

template <unsigned rem, unsigned... digits>
struct explode : explode<rem / 10, rem % 10, digits...> {};

template <unsigned... digits>
struct explode<0, digits...> : to_chars<digits...> {};
}  // namespace detail

template <unsigned num>
struct num_to_string : detail::explode<num / 10, num % 10> {};

inline asio::const_buffer to_buffer(status_type status) {
  switch (status) {
    case status_type::switching_protocols:
      return asio::buffer(switching_protocols.data(),
                          switching_protocols.length());
    case status_type::ok:
      return asio::buffer(rep_ok.data(), rep_ok.length());
    case status_type::created:
      return asio::buffer(rep_created.data(), rep_created.length());
    case status_type::accepted:
      return asio::buffer(rep_accepted.data(), rep_created.length());
    case status_type::no_content:
      return asio::buffer(rep_no_content.data(), rep_no_content.length());
    case status_type::partial_content:
      return asio::buffer(rep_partial_content.data(),
                          rep_partial_content.length());
    case status_type::multiple_choices:
      return asio::buffer(rep_multiple_choices.data(),
                          rep_multiple_choices.length());
    case status_type::moved_permanently:
      return asio::buffer(rep_moved_permanently.data(),
                          rep_moved_permanently.length());
    case status_type::temporary_redirect:
      return asio::buffer(rep_temporary_redirect.data(),
                          rep_temporary_redirect.length());
    case status_type::moved_temporarily:
      return asio::buffer(rep_moved_temporarily.data(),
                          rep_moved_temporarily.length());
    case status_type::not_modified:
      return asio::buffer(rep_not_modified.data(), rep_not_modified.length());
    case status_type::bad_request:
      return asio::buffer(rep_bad_request.data(), rep_bad_request.length());
    case status_type::unauthorized:
      return asio::buffer(rep_unauthorized.data(), rep_unauthorized.length());
    case status_type::forbidden:
      return asio::buffer(rep_forbidden.data(), rep_forbidden.length());
    case status_type::not_found:
      return asio::buffer(rep_not_found.data(), rep_not_found.length());
    case status_type::conflict:
      return asio::buffer(rep_conflict.data(), rep_conflict.length());
    case status_type::internal_server_error:
      return asio::buffer(rep_internal_server_error.data(),
                          rep_internal_server_error.length());
    case status_type::not_implemented:
      return asio::buffer(rep_not_implemented.data(),
                          rep_not_implemented.length());
    case status_type::bad_gateway:
      return asio::buffer(rep_bad_gateway.data(), rep_bad_gateway.length());
    case status_type::service_unavailable:
      return asio::buffer(rep_service_unavailable.data(),
                          rep_service_unavailable.length());
    default:
      return asio::buffer(rep_internal_server_error.data(),
                          rep_internal_server_error.length());
  }
}

inline constexpr std::string_view to_rep_string(status_type status) {
  switch (status) {
    case cinatra::status_type::switching_protocols:
      return switching_protocols;
      break;
    case cinatra::status_type::ok:
      return rep_ok;
      break;
    case cinatra::status_type::created:
      return rep_created;
      break;
    case cinatra::status_type::accepted:
      return rep_accepted;
      break;
    case cinatra::status_type::no_content:
      return rep_no_content;
      break;
    case cinatra::status_type::partial_content:
      return rep_partial_content;
      break;
    case cinatra::status_type::multiple_choices:
      return rep_multiple_choices;
      break;
    case cinatra::status_type::moved_permanently:
      return rep_moved_permanently;
      break;
    case cinatra::status_type::moved_temporarily:
      return rep_moved_temporarily;
      break;
    case cinatra::status_type::not_modified:
      return rep_not_modified;
      break;
    case cinatra::status_type::temporary_redirect:
      return rep_temporary_redirect;
      break;
    case cinatra::status_type::bad_request:
      return rep_bad_request;
      break;
    case cinatra::status_type::unauthorized:
      return rep_unauthorized;
      break;
    case cinatra::status_type::forbidden:
      return rep_forbidden;
      break;
    case cinatra::status_type::not_found:
      return rep_not_found;
      break;
    case cinatra::status_type::conflict:
      return rep_conflict;
      break;
    case cinatra::status_type::internal_server_error:
      return rep_internal_server_error;
      break;
    case cinatra::status_type::not_implemented:
      return rep_not_implemented;
      break;
    case cinatra::status_type::bad_gateway:
      return rep_bad_gateway;
      break;
    case cinatra::status_type::service_unavailable:
      return rep_service_unavailable;
      break;
    default:
      return rep_not_implemented;
      break;
  }
}

inline std::string_view to_string(status_type status) {
  switch (status) {
    case status_type::ok:
      return ok;
    case status_type::created:
      return created;
    case status_type::accepted:
      return accepted;
    case status_type::no_content:
      return no_content;
    case status_type::multiple_choices:
      return multiple_choices;
    case status_type::moved_permanently:
      return moved_permanently;
    case status_type::moved_temporarily:
      return moved_temporarily;
    case status_type::temporary_redirect:
      return temporary_redirect;
    case status_type::not_modified:
      return not_modified;
    case status_type::bad_request:
      return bad_request;
    case status_type::unauthorized:
      return unauthorized;
    case status_type::forbidden:
      return forbidden;
    case status_type::not_found:
      return not_found;
    case status_type::conflict:
      return conflict;
    case status_type::internal_server_error:
      return internal_server_error;
    case status_type::not_implemented:
      return not_implemented;
    case status_type::bad_gateway:
      return bad_gateway;
    case status_type::service_unavailable:
      return service_unavailable;
    default:
      return internal_server_error;
  }
}
}  // namespace cinatra
