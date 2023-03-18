#pragma once
#include <asio/streambuf.hpp>
#include <atomic>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <thread>
#include <unordered_map>

#include "asio_util/asio_coro_util.hpp"
#include "async_simple/Future.h"
#include "async_simple/coro/FutureAwaiter.h"
#include "async_simple/coro/Lazy.h"
#include "http_parser.hpp"
#include "response_cv.hpp"
#include "uri.hpp"
#include "websocket.hpp"

namespace cinatra {
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
enum class ClientInjectAction {
  none,
  response_error,
  header_error,
  chunk_error,
  write_failed,
  read_failed,
};
inline ClientInjectAction inject_response_valid = ClientInjectAction::none;
inline ClientInjectAction inject_header_valid = ClientInjectAction::none;
inline ClientInjectAction inject_chunk_valid = ClientInjectAction::none;
inline ClientInjectAction inject_write_failed = ClientInjectAction::none;
inline ClientInjectAction inject_read_failed = ClientInjectAction::none;
#endif

struct resp_data {
  std::error_code net_err;
  int status;
  std::string_view resp_body;
  std::vector<std::pair<std::string, std::string>> resp_headers;
  bool eof;
};

template <typename Stream>
struct req_context {
  req_content_type content_type = req_content_type::none;
  std::string req_str;
  std::string content;
  Stream stream;
};

struct multipart_t {
  std::string filename;
  std::string content;
  size_t size = 0;
};

class coro_http_client {
 public:
  coro_http_client()
      : socket_(io_ctx_),
        executor_(io_ctx_.get_executor()),
        timer_(io_ctx_.get_executor()) {
    work_ = std::make_unique<asio::io_context::work>(io_ctx_);
    io_thd_ = std::thread([this] {
      io_ctx_.run();
    });
  }

  ~coro_http_client() {
    async_close();
    work_ = nullptr;
    if (io_thd_.joinable()) {
      io_thd_.join();
    }

    std::cout << "client quit\n";
  }

  void async_close() {
    if (has_closed_)
      return;

    io_ctx_.dispatch([this] {
      close_socket();
    });
  }

#ifdef CINATRA_ENABLE_SSL
  [[nodiscard]] bool init_ssl(const std::string &base_path,
                              const std::string &cert_file,
                              int verify_mode = asio::ssl::verify_none,
                              const std::string &domain = "localhost") {
    try {
      ssl_init_ret_ = false;
      printf("init ssl: %s", domain.data());
      auto full_cert_file = std::filesystem::path(base_path).append(cert_file);
      printf("current path %s",
             std::filesystem::current_path().string().data());
      if (std::filesystem::exists(full_cert_file)) {
        printf("load %s", full_cert_file.string().data());
        ssl_ctx_.load_verify_file(full_cert_file.string());
      }
      else {
        printf("no certificate file %s", full_cert_file.string().data());
        return ssl_init_ret_;
      }

      ssl_ctx_.set_verify_mode(verify_mode);

      // ssl_ctx_.add_certificate_authority(asio::buffer(CA_PEM));

      ssl_ctx_.set_verify_callback(asio::ssl::host_name_verification(domain));
      ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              socket_, ssl_ctx_);
      use_ssl_ = true;
      ssl_init_ret_ = true;
    } catch (std::exception &e) {
      printf("init ssl failed: %s", e.what());
    }
    return ssl_init_ret_;
  }
#endif

  bool has_closed() { return has_closed_; }

  bool add_header(std::string key, std::string val) {
    if (key.empty())
      return false;

    if (key == "Host")
      return false;

    if (auto it = std::find_if(req_headers_.begin(), req_headers_.end(),
                               [&key](auto &item) {
                                 return item.first == key;
                               });
        it != req_headers_.end()) {
      return false;
    }

    req_headers_.emplace_back(std::move(key), std::move(val));
    return true;
  }

  void set_ws_sec_key(std::string sec_key) { ws_sec_key_ = std::move(sec_key); }

  async_simple::coro::Lazy<bool> async_connect(std::string uri) {
    resp_data data{};
    auto [r, u] = handle_uri(data, uri);
    if (!r) {
      std::cout << "url error";
      co_return false;
    }

    req_context<std::string> ctx{};
    if (u.is_websocket()) {
      // build websocket http header
      add_header("Upgrade", "websocket");
      add_header("Connection", "Upgrade");
      if (ws_sec_key_.empty()) {
        ws_sec_key_ = "s//GYHa/XO7Hd2F2eOGfyA==";  // provide a random string.
      }
      add_header("Sec-WebSocket-Key", ws_sec_key_);
      add_header("Sec-WebSocket-Version", "13");
    }

    data = co_await async_request(std::move(uri), http_method::GET,
                                  std::move(ctx));
    async_read_ws().start([](auto &&) {
    });
    co_return !data.net_err;
  }

  async_simple::coro::Lazy<resp_data> async_send_ws(std::string msg,
                                                    bool need_mask = true,
                                                    opcode op = opcode::text) {
    resp_data data{};

    websocket ws{};
    if (op == opcode::close) {
      msg = ws.format_close_payload(close_code::normal, msg.data(), msg.size());
    }

    std::string encode_header = ws.encode_frame(msg, op, need_mask);
    std::vector<asio::const_buffer> buffers{
        asio::buffer(encode_header.data(), encode_header.size()),
        asio::buffer(msg.data(), msg.size())};

    auto [ec, _] = co_await async_write(buffers);
    if (ec) {
      data.net_err = ec;
      data.status = 404;
    }

    co_return data;
  }

  async_simple::coro::Lazy<resp_data> async_send_ws_close(
      std::string msg = "") {
    return async_send_ws(std::move(msg), false, opcode::close);
  }

  void on_ws_msg(std::function<void(resp_data)> on_ws_msg) {
    on_ws_msg_ = std::move(on_ws_msg);
  }
  void on_ws_close(std::function<void(std::string_view)> on_ws_close) {
    on_ws_close_ = std::move(on_ws_close);
  }

  async_simple::coro::Lazy<resp_data> async_get(std::string uri) {
    req_context<std::string> ctx{};
    resp_data data{};
    data = co_await async_request(std::move(uri), http_method::GET,
                                  std::move(ctx));

    if (redirect_uri_.empty() || !is_redirect(data)) {
      co_return data;
    }
    else {
      if (enable_follow_redirect_)
        data = co_await async_request(std::move(redirect_uri_),
                                      http_method::GET, std::move(ctx));
      co_return data;
    }
  }

  resp_data get(std::string uri) {
    return async_simple::coro::syncAwait(async_get(std::move(uri)));
  }

  async_simple::coro::Lazy<resp_data> async_post(
      std::string uri, std::string content, req_content_type content_type) {
    req_context<std::string> ctx{content_type, "", std::move(content)};
    return async_request(std::move(uri), http_method::POST, std::move(ctx));
  }

  resp_data post(std::string uri, std::string content,
                 req_content_type content_type) {
    return async_simple::coro::syncAwait(
        async_post(std::move(uri), std::move(content), content_type));
  }

  bool add_str_part(std::string name, std::string content) {
    size_t size = content.size();
    return form_data_
        .emplace(std::move(name), multipart_t{"", std::move(content), size})
        .second;
  }

  bool add_file_part(std::string name, std::string filename) {
    if (form_data_.find(name) != form_data_.end()) {
      std::cout << "name already exist\n";
      return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
      std::cout << "open file failed, "
                << std::filesystem::current_path().string() << std::endl;
      return false;
    }

    size_t file_size = std::filesystem::file_size(filename);
    form_data_.emplace(std::move(name),
                       multipart_t{std::move(filename), "", file_size});
    return true;
  }

  void set_max_single_part_size(size_t size) { max_single_part_size_ = size; }

  async_simple::Promise<async_simple::Unit> start_timer() {
    async_simple::Promise<async_simple::Unit> promise;
    if (enable_timeout_) {
      timeout(timer_, promise, "request timer canceled")
          .via(&executor_)
          .detach();
    }

    return promise;
  }

  async_simple::coro::Lazy<std::error_code> wait_timer(
      async_simple::Promise<async_simple::Unit> &promise) {
    if (!enable_timeout_) {
      co_return std::error_code{};
    }
    std::error_code err_code;
    timer_.cancel(err_code);
    co_await promise.getFuture();
    if (is_timeout_) {
      co_return std::make_error_code(std::errc::timed_out);
    }

    co_return std::error_code{};
  }

  async_simple::coro::Lazy<resp_data> async_upload(std::string uri) {
    std::shared_ptr<int> guard(nullptr, [this](auto) {
      req_headers_.clear();
      form_data_.clear();
    });
    if (form_data_.empty()) {
      std::cout << "no multipart\n";
      co_return resp_data{{}, 404};
    }

    req_context<std::string> ctx{req_content_type::multipart, "", ""};
    resp_data data{};
    auto [ok, u] = handle_uri(data, uri);
    if (!ok) {
      co_return resp_data{{}, 404};
    }

    size_t content_len = multipart_content_len();

    add_header("Content-Length", std::to_string(content_len));

    std::string header_str = build_request_header(u, http_method::POST, ctx);

    std::error_code ec{};
    size_t size = 0;

    auto promise = start_timer();

    data = co_await connect(u);
    if (data.net_err) {
      co_return data;
    }

    std::tie(ec, size) = co_await async_write(asio::buffer(header_str));
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
    if (inject_write_failed == ClientInjectAction::write_failed) {
      ec = std::make_error_code(std::errc::not_connected);
    }
#endif
    if (ec) {
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
      inject_write_failed = ClientInjectAction::none;
#endif
      std::cout << ec.message() << "\n";
      co_return resp_data{ec, 404};
    }

    for (auto &[key, part] : form_data_) {
      data = co_await send_single_part(key, part);

      if (data.net_err) {
        co_return data;
      }
    }

    std::string last_part;
    last_part.append("--").append(BOUNDARY).append("--").append(CRCF);
    if (std::tie(ec, size) = co_await async_write(asio::buffer(last_part));
        ec) {
      co_return resp_data{ec, 404};
    }

    bool is_keep_alive = true;
    data = co_await handle_read(ec, size, is_keep_alive, std::move(ctx));
    if (auto errc = co_await wait_timer(promise); errc) {
      ec = errc;
    }

    handle_result(data, ec, is_keep_alive);
    co_return data;
  }

  async_simple::coro::Lazy<resp_data> async_upload(std::string uri,
                                                   std::string name,
                                                   std::string filename) {
    if (!add_file_part(std::move(name), std::move(filename))) {
      std::cout << "open file failed or duplicate test names\n";
      co_return resp_data{{}, 404};
    }
    co_return co_await async_upload(std::move(uri));
  }

  async_simple::coro::Lazy<resp_data> async_download(std::string uri,
                                                     std::string filename,
                                                     std::string range = "") {
    resp_data data{};
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file) {
      data.net_err = std::make_error_code(std::errc::no_such_file_or_directory);
      data.status = 404;
      co_return data;
    }

    req_context<std::ofstream> ctx{};
    if (range.empty()) {
      ctx = {req_content_type::none, "", "", std::move(file)};
    }
    else {
      std::string req_str = "Range: bytes=";
      req_str.append(range).append(CRCF);
      ctx = {req_content_type::none, std::move(req_str), {}, std::move(file)};
    }

    data = co_await async_request(std::move(uri), http_method::GET,
                                  std::move(ctx));

    co_return data;
  }

  resp_data download(std::string uri, std::string filename,
                     std::string range = "") {
    return async_simple::coro::syncAwait(
        async_download(std::move(uri), std::move(filename), std::move(range)));
  }

  async_simple::coro::Lazy<resp_data> async_request(std::string uri,
                                                    http_method method,
                                                    auto ctx) {
    resp_data data{};

    std::error_code ec{};
    size_t size = 0;
    bool is_keep_alive = false;

    auto promise = start_timer();

    do {
      auto [ok, u] = handle_uri(data, uri);
      if (!ok) {
        break;
      }

      if (has_closed_) {
        std::string host = proxy_host_.empty() ? u.get_host() : proxy_host_;
        std::string port = proxy_port_.empty() ? u.get_port() : proxy_port_;
        if (ec = co_await asio_util::async_connect(io_ctx_.get_executor(),
                                                   socket_, host, port);
            ec) {
          break;
        }

        if (u.is_ssl) {
          if (ec = co_await handle_shake(); ec) {
            break;
          }
        }
        has_closed_ = false;
      }

      std::string write_msg = prepare_request_str(u, method, ctx);

      if (std::tie(ec, size) = co_await async_write(asio::buffer(write_msg));
          ec) {
        break;
      }

      data = co_await handle_read(ec, size, is_keep_alive, std::move(ctx));
    } while (0);

    if (auto errc = co_await wait_timer(promise); errc) {
      ec = errc;
    }

    handle_result(data, ec, is_keep_alive);
    co_return data;
  }

  async_simple::coro::Lazy<std::error_code> handle_shake() {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      if (ssl_stream_ == nullptr) {
        co_return std::make_error_code(std::errc::not_a_stream);
      }

      auto ec = co_await asio_util::async_handshake(
          ssl_stream_, asio::ssl::stream_base::client);
      if (ec) {
        std::cout << "handle failed " << ec.message() << "\n";
      }
      co_return ec;
    }
    else {
      co_return std::error_code{};
    }
#else
    // please open CINATRA_ENABLE_SSL before request https!
    co_return std::make_error_code(std::errc::protocol_error);
#endif
  }

  inline void set_proxy(const std::string &host, const std::string &port) {
    proxy_host_ = host;
    proxy_port_ = port;
  }

  inline void set_proxy_basic_auth(const std::string &username,
                                   const std::string &password) {
    proxy_basic_auth_username_ = username;
    proxy_basic_auth_password_ = password;
  }

  inline void set_proxy_bearer_token_auth(const std::string &token) {
    proxy_bearer_token_auth_token_ = token;
  }

  inline void enable_auto_redirect(bool enable_follow_redirect) {
    enable_follow_redirect_ = enable_follow_redirect;
  }

  std::string get_redirect_uri() { return redirect_uri_; }

  bool is_redirect(resp_data &data) {
    if (data.status > 299 && data.status <= 399)
      return true;
    return false;
  }

  inline void set_timeout(
      std::chrono::steady_clock::duration timeout_duration) {
    enable_timeout_ = true;
    timeout_duration_ = timeout_duration;
  }

 private:
  std::pair<bool, uri_t> handle_uri(resp_data &data, const std::string &uri) {
    uri_t u;
    if (!u.parse_from(uri.data())) {
      data.net_err = std::make_error_code(std::errc::protocol_error);
      data.status = 404;
      return {false, {}};
    }

    // construct proxy request uri
    construct_proxy_uri(u);

    return {true, u};
  }

  void construct_proxy_uri(uri_t &u) {
    if (!proxy_host_.empty() && !proxy_port_.empty()) {
      if (!proxy_request_uri_.empty())
        proxy_request_uri_.clear();
      if (u.get_port() == "http") {
        proxy_request_uri_ += "http://" + u.get_host() + ":";
        proxy_request_uri_ += "80";
      }
      else if (u.get_port() == "https") {
        proxy_request_uri_ += "https://" + u.get_host() + ":";
        proxy_request_uri_ += "443";
      }
      else {
        // all be http
        proxy_request_uri_ += "http://" + u.get_host() + ":";
        proxy_request_uri_ += u.get_port();
      }
      proxy_request_uri_ += u.get_path();
      u.path = std::string_view(proxy_request_uri_);
    }
  }

  std::string build_request_header(const uri_t &u, http_method method,
                                   const auto &ctx) {
    std::string req_str(method_name(method));

    req_str.append(" ").append(u.get_path());
    if (!u.query.empty()) {
      req_str.append("?").append(u.query);
    }

    req_str.append(" HTTP/1.1\r\nHost:").append(u.host).append("\r\n");
    auto type_str = get_content_type_str(ctx.content_type);
    if (!type_str.empty()) {
      if (ctx.content_type == req_content_type::multipart) {
        type_str.append(BOUNDARY);
      }
      req_headers_.emplace_back("Content-Type", std::move(type_str));
    }

    bool has_connection = false;
    // add user headers
    if (!req_headers_.empty()) {
      for (auto &pair : req_headers_) {
        if (pair.first == "Connection") {
          has_connection = true;
        }
        req_str.append(pair.first)
            .append(": ")
            .append(pair.second)
            .append("\r\n");
      }
    }

    if (!has_connection) {
      req_str.append("Connection: keep-alive\r\n");
    }

    if (!proxy_basic_auth_username_.empty() &&
        !proxy_basic_auth_password_.empty()) {
      std::string basic_auth_str = "Proxy-Authorization: Basic ";
      std::string basic_base64_str = base64_encode(
          proxy_basic_auth_username_ + ":" + proxy_basic_auth_password_);
      req_str.append(basic_auth_str).append(basic_base64_str).append(CRCF);
    }

    if (!proxy_bearer_token_auth_token_.empty()) {
      std::string bearer_token_str = "Proxy-Authorization: Bearer ";
      req_str.append(bearer_token_str)
          .append(proxy_bearer_token_auth_token_)
          .append(CRCF);
    }

    if (!ctx.req_str.empty())
      req_str.append(ctx.req_str);

    size_t content_len = ctx.content.size();
    bool should_add = false;
    if (content_len > 0) {
      should_add = true;
    }
    else {
      if (method == http_method::POST &&
          ctx.content_type != req_content_type::multipart)
        should_add = true;
    }

    if (should_add) {
      char buf[32];
      auto [ptr, ec] = std::to_chars(buf, buf + 32, content_len);
      req_str.append("Content-Length: ")
          .append(std::string_view(buf, ptr - buf))
          .append("\r\n");
    }

    req_str.append("\r\n");
    return req_str;
  }

  std::string prepare_request_str(const uri_t &u, http_method method,
                                  const auto &ctx) {
    std::string req_str = build_request_header(u, method, ctx);

    if (!ctx.content.empty())
      req_str.append(std::move(ctx.content));

    return req_str;
  }

  std::error_code handle_header(resp_data &data, http_parser &parser,
                                size_t header_size) {
    // parse header
    const char *data_ptr = asio::buffer_cast<const char *>(read_buf_.data());

    int parse_ret = parser.parse_response(data_ptr, header_size, 0);
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
    if (inject_response_valid == ClientInjectAction::response_error) {
      parse_ret = -1;
    }
#endif
    if (parse_ret < 0) {
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
      inject_response_valid = ClientInjectAction::none;
#endif
      return std::make_error_code(std::errc::protocol_error);
    }
    read_buf_.consume(header_size);  // header size
    data.resp_headers = get_headers(parser);
    data.status = parser.status();
    return {};
  }

  async_simple::coro::Lazy<resp_data> handle_read(std::error_code &ec,
                                                  size_t &size,
                                                  bool &is_keep_alive,
                                                  auto ctx) {
    resp_data data{};
    do {
      if (std::tie(ec, size) = co_await async_read_until(read_buf_, TWO_CRCF);
          ec) {
        break;
      }

      http_parser parser;
      ec = handle_header(data, parser, size);
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
      if (inject_header_valid == ClientInjectAction::header_error) {
        ec = std::make_error_code(std::errc::protocol_error);
      }
#endif
      if (ec) {
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
        inject_header_valid = ClientInjectAction::none;
#endif
        break;
      }

      is_keep_alive = parser.keep_alive();
      bool is_ranges = parser.is_ranges();
      if (parser.is_chunked()) {
        is_keep_alive = true;
        ec = co_await handle_chunked(data, std::move(ctx));
        break;
      }

      redirect_uri_.clear();
      bool is_redirect = parser.is_location();
      if (is_redirect)
        redirect_uri_ = parser.get_header_value("Location");

      size_t content_len = (size_t)parser.body_len();

      if ((size_t)parser.body_len() <= read_buf_.size()) {
        // Now get entire content, additional data will discard.
        handle_entire_content(data, content_len, is_ranges, ctx);
        break;
      }

      // read left part of content.
      size_t size_to_read = content_len - read_buf_.size();
      if (std::tie(ec, size) = co_await async_read(read_buf_, size_to_read);
          ec) {
        break;
      }

      // Now get entire content, additional data will discard.
      handle_entire_content(data, content_len, is_ranges, ctx);
    } while (0);

    co_return data;
  }

  void handle_entire_content(resp_data &data, size_t content_len,
                             bool is_ranges, auto &ctx) {
    if (content_len > 0) {
      if (is_ranges) {
        auto data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
        if constexpr (std::is_same_v<
                          std::ofstream,
                          std::remove_cvref_t<decltype(ctx.stream)>>) {
          ctx.stream.write(data_ptr, content_len);
        }
      }

      auto data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      std::string_view reply(data_ptr, content_len);
      data.resp_body = reply;

      read_buf_.consume(content_len);
    }
    data.eof = (read_buf_.size() == 0);
  }

  void handle_result(resp_data &data, std::error_code ec, bool is_keep_alive) {
    if (ec) {
      close_socket();
      data.net_err = ec;
      data.status = 404;
      std::cout << ec.message() << "\n";
    }
    else {
      if (!is_keep_alive) {
        close_socket();
      }
    }
  }

  template <typename Stream>
  async_simple::coro::Lazy<std::error_code> handle_chunked(
      resp_data &data, req_context<Stream> ctx) {
    std::error_code ec{};
    size_t size = 0;
    while (true) {
      if (std::tie(ec, size) = co_await async_read_until(read_buf_, CRCF); ec) {
        break;
      }

#ifdef INJECT_FOR_HTTP_CLIENT_TEST
      if (inject_read_failed == ClientInjectAction::read_failed) {
        ec = std::make_error_code(std::errc::not_connected);
      }
      if (ec) {
        inject_read_failed = ClientInjectAction::none;
        break;
      }
#endif

      size_t buf_size = read_buf_.size();
      size_t additional_size = buf_size - size;
      const char *data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      std::string_view size_str(data_ptr, size - CRCF.size());
      auto chunk_size = hex_to_int(size_str);
      read_buf_.consume(size);
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
      if (inject_chunk_valid == ClientInjectAction::chunk_error) {
        chunk_size = -1;
      }
#endif
      if (chunk_size < 0) {
#ifdef INJECT_FOR_HTTP_CLIENT_TEST
        inject_chunk_valid = ClientInjectAction::none;
#endif
        std::cout << "bad chunked size\n";
        ec = asio::error::make_error_code(
            asio::error::basic_errors::invalid_argument);
        break;
      }

      if (chunk_size == 0) {
        // all finished, no more data
        read_buf_.consume(CRCF.size());
        data.status = 200;
        data.eof = true;
        break;
      }

      if (additional_size < size_t(chunk_size + 2)) {
        // not a complete chunk, read left chunk data.
        size_t size_to_read = chunk_size + 2 - additional_size;
        if (std::tie(ec, size) = co_await async_read(read_buf_, size_to_read);
            ec) {
          break;
        }
      }

      data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      if constexpr (std::is_same_v<std::ofstream,
                                   std::remove_cvref_t<Stream>>) {
        ctx.stream.write(data_ptr, chunk_size);
      }

      read_buf_.consume(chunk_size + CRCF.size());
    }
    co_return ec;
  }

  async_simple::coro::Lazy<resp_data> connect(const uri_t &u) {
    if (has_closed_) {
      std::string host = proxy_host_.empty() ? u.get_host() : proxy_host_;
      std::string port = proxy_port_.empty() ? u.get_port() : proxy_port_;
      if (auto ec = co_await asio_util::async_connect(io_ctx_.get_executor(),
                                                      socket_, host, port);
          ec) {
        co_return resp_data{ec, 404};
      }

      if (u.is_ssl) {
        if (auto ec = co_await handle_shake(); ec) {
          co_return resp_data{ec, 404};
        }
      }
      has_closed_ = false;
    }

    co_return resp_data{};
  }

  size_t multipart_content_len() {
    size_t content_len = 0;
    for (auto &[key, part] : form_data_) {
      content_len += 75;
      content_len += key.size() + 1;
      if (!part.filename.empty()) {
        content_len += (12 + part.filename.size() + 1);
        auto ext = std::filesystem::path(part.filename).extension().string();
        if (auto it = g_content_type_map.find(ext);
            it != g_content_type_map.end()) {
          content_len += (14 + it->second.size());
        }
      }

      content_len += 4;

      content_len += (part.size + 2);
    }
    content_len += (6 + BOUNDARY.size());
    return content_len;
  }

  async_simple::coro::Lazy<resp_data> send_single_part(
      const std::string &key, const multipart_t &part) {
    std::string part_content_head;
    part_content_head.append("--").append(BOUNDARY).append(CRCF);
    part_content_head.append("Content-Disposition: form-data; name=\"");
    part_content_head.append(key).append("\"");
    bool is_file = !part.filename.empty();
    std::string short_name =
        std::filesystem::path(part.filename).filename().string();
    if (is_file) {
      part_content_head.append("; filename=\"").append(short_name).append("\"");
      auto ext = std::filesystem::path(short_name).extension().string();
      if (auto it = g_content_type_map.find(ext);
          it != g_content_type_map.end()) {
        part_content_head.append("Content-Type: ").append(it->second);
      }

      std::error_code ec;
      if (!std::filesystem::exists(part.filename, ec)) {
        co_return resp_data{
            std::make_error_code(std::errc::no_such_file_or_directory), 404};
      }
    }
    part_content_head.append(TWO_CRCF);
    if (auto [ec, size] = co_await async_write(asio::buffer(part_content_head));
        ec) {
      co_return resp_data{ec, 404};
    }

    if (is_file) {
      std::ifstream file(part.filename, std::ios::binary);
      assert(file.is_open());

      size_t left_size = part.size;
      size_t size_to_read = left_size;
      std::string file_data;
      while (left_size > 0) {
        size_to_read = (std::min)(left_size, max_single_part_size_);
        file_data.resize(size_to_read);
        left_size -= file.read(file_data.data(), file_data.size()).gcount();
        if (auto [ec, size] = co_await async_write(asio::buffer(file_data));
            ec) {
          co_return resp_data{ec, 404};
        }
      }
    }
    else {
      if (auto [ec, size] = co_await async_write(asio::buffer(part.content));
          ec) {
        co_return resp_data{ec, 404};
      }
    }

    if (auto [ec, size] = co_await async_write(asio::buffer(CRCF)); ec) {
      co_return resp_data{ec, 404};
    }

    co_return resp_data{{}, 200};
  }

  std::vector<std::pair<std::string, std::string>> get_headers(
      http_parser &parser) {
    std::vector<std::pair<std::string, std::string>> resp_headers;

    auto [headers, num_headers] = parser.get_headers();
    for (size_t i = 0; i < num_headers; i++) {
      resp_headers.emplace_back(
          std::string(headers[i].name, headers[i].name_len),
          std::string(headers[i].value, headers[i].value_len));
    }

    return resp_headers;
  }

  async_simple::coro::Lazy<void> async_read_ws() {
    resp_data data{};

    read_buf_.consume(read_buf_.size());
    size_t header_size = 2;

    websocket ws{};
    while (true) {
      if (auto [ec, _] = co_await async_read(read_buf_, header_size); ec) {
        data.net_err = ec;
        data.status = 404;
        if (on_ws_msg_)
          on_ws_msg_(data);
        co_return;
      }

      const char *data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      auto ret = ws.parse_header(data_ptr, header_size, false);
      if (ret == -2) {
        header_size += ws.left_header_len();
        continue;
      }
      frame_header *header = (frame_header *)data_ptr;
      bool is_close_frame = header->opcode == opcode::close;

      read_buf_.consume(header_size);

      size_t payload_len = ws.payload_length();
      if (payload_len > read_buf_.size()) {
        size_t size_to_read = payload_len - read_buf_.size();
        if (auto [ec, size] = co_await async_read(read_buf_, size_to_read);
            ec) {
          data.net_err = ec;
          data.status = 404;
          if (on_ws_msg_)
            on_ws_msg_(data);
          co_return;
        }
      }

      data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      if (is_close_frame) {
        payload_len -= 4;
        data_ptr += sizeof(uint16_t);
      }

      data.status = 200;
      data.resp_body = {data_ptr, payload_len};

      read_buf_.consume(read_buf_.size());
      header_size = 2;

      if (is_close_frame) {
        if (on_ws_close_)
          on_ws_close_(data.resp_body);
        co_await async_send_ws("close", false, opcode::close);
        async_close();
        co_return;
      }
      if (on_ws_msg_)
        on_ws_msg_(data);
    }
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      AsioBuffer &buffer, size_t size_to_read) noexcept {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return asio_util::async_read(*ssl_stream_, buffer, size_to_read);
    }
    else {
#endif
      return asio_util::async_read(socket_, buffer, size_to_read);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
      AsioBuffer &&buffer) {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return asio_util::async_write(*ssl_stream_, buffer);
    }
    else {
#endif
      return asio_util::async_write(socket_, buffer);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_until(
      AsioBuffer &buffer, asio::string_view delim) noexcept {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return asio_util::async_read_until(*ssl_stream_, buffer, delim);
    }
    else {
#endif
      return asio_util::async_read_until(socket_, buffer, delim);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  void close_socket() {
    std::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    has_closed_ = true;
  }

  async_simple::coro::Lazy<bool> timeout(auto &timer, auto &promise,
                                         std::string msg) {
    timer.expires_after(timeout_duration_);
    is_timeout_ = co_await timer.async_await();
    if (!is_timeout_) {
      promise.setValue(async_simple::Unit());
      std::cout << msg << '\n';
      co_return false;
    }
    std::cout << "request timeout\n";
    close_socket();
    promise.setValue(async_simple::Unit());
    co_return true;
  }

  asio::io_context io_ctx_;
  asio::ip::tcp::socket socket_;
  asio_util::ExecutorWrapper<> executor_;
  std::unique_ptr<asio::io_context::work> work_;
  asio_util::period_timer timer_;
  std::thread io_thd_;

  std::atomic<bool> has_closed_ = true;
  asio::streambuf read_buf_;

  std::vector<std::pair<std::string, std::string>> req_headers_;

  std::string proxy_request_uri_ = "";
  std::string proxy_host_;
  std::string proxy_port_;

  std::string proxy_basic_auth_username_;
  std::string proxy_basic_auth_password_;

  std::string proxy_bearer_token_auth_token_;

  std::map<std::string, multipart_t> form_data_;
  size_t max_single_part_size_ = 1024 * 1024;

  std::function<void(resp_data)> on_ws_msg_;
  std::function<void(std::string_view)> on_ws_close_;
  std::string ws_sec_key_;

#ifdef CINATRA_ENABLE_SSL
  asio::ssl::context ssl_ctx_{asio::ssl::context::sslv23};
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool ssl_init_ret_ = true;
  bool use_ssl_ = false;
#endif
  std::string redirect_uri_;
  bool enable_follow_redirect_ = false;

  bool is_timeout_ = false;
  bool enable_timeout_ = false;
  std::chrono::steady_clock::duration timeout_duration_ =
      std::chrono::seconds(60);
};
}  // namespace cinatra
