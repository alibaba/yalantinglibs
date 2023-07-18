#pragma once
#include <atomic>
#include <cassert>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>

#include "asio/dispatch.hpp"
#include "asio/error.hpp"
#include "asio/streambuf.hpp"
#include "async_simple/Future.h"
#include "async_simple/Unit.h"
#include "async_simple/coro/FutureAwaiter.h"
#include "async_simple/coro/Lazy.h"
#include "http_parser.hpp"
#include "response_cv.hpp"
#include "uri.hpp"
#include "websocket.hpp"
#include "ylt/coro_io/coro_file.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"

namespace coro_io {
template <typename T, typename U>
class client_pool;
}
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
#ifdef BENCHMARK_TEST
  uint64_t total;
#endif
};

template <typename String = std::string>
struct req_context {
  req_content_type content_type = req_content_type::none;
  std::string req_str;
  String content;
  std::shared_ptr<coro_io::coro_file> stream = nullptr;
};

struct multipart_t {
  std::string filename;
  std::string content;
  size_t size = 0;
};

class simple_buffer {
 public:
  inline static constexpr size_t sbuf_init_size = 4096;
  simple_buffer(size_t init_size = sbuf_init_size)
      : size_(0), alloc_size_(init_size) {
    if (init_size == 0) {
      data_ = nullptr;
    }
    else {
      data_ = (char *)::malloc(init_size);
      if (!data_) {
        throw std::bad_alloc();
      }
    }
  }

  ~simple_buffer() { ::free(data_); }

  simple_buffer(const simple_buffer &) = delete;
  simple_buffer &operator=(const simple_buffer &) = delete;

  simple_buffer(simple_buffer &&other)
      : size_(other.size_), data_(other.data_), alloc_size_(other.alloc_size_) {
    other.size_ = other.alloc_size_ = 0;
    other.data_ = nullptr;
  }

  simple_buffer &operator=(simple_buffer &&other) {
    ::free(data_);

    size_ = other.size_;
    alloc_size_ = other.alloc_size_;
    data_ = other.data_;

    other.size_ = other.alloc_size_ = 0;
    other.data_ = nullptr;

    return *this;
  }

  char *data() { return data_; }

  const char *data() const { return data_; }

  size_t size() const { return size_; }
  size_t alloc_size() const { return alloc_size_; }

  char *release() {
    char *tmp = data_;
    size_ = 0;
    data_ = nullptr;
    alloc_size_ = 0;
    return tmp;
  }

  void init(size_t len) {
    if (alloc_size_ - size_ >= len) {
      size_ = len;
      return;
    }

    size_t nsize = (alloc_size_ > 0) ? alloc_size_ * 2 : sbuf_init_size;

    while (nsize < size_ + len) {
      size_t tmp_nsize = nsize * 2;
      if (tmp_nsize <= nsize) {
        nsize = size_ + len;
        break;
      }
      nsize = tmp_nsize;
    }

    void *tmp = ::realloc(data_, nsize);
    if (!tmp) {
      throw std::bad_alloc();
    }

    data_ = static_cast<char *>(tmp);
    alloc_size_ = nsize;
    size_ = nsize;
  }

 private:
  size_t size_;
  size_t alloc_size_;
  char *data_;
};

class coro_http_client {
 public:
  struct config {
    std::optional<std::chrono::steady_clock::duration> conn_timeout_duration;
    std::optional<std::chrono::steady_clock::duration> req_timeout_duration;
    std::string sec_key;
    size_t max_single_part_size;
    std::string proxy_host;
    std::string proxy_port;
    std::string proxy_auth_username;
    std::string proxy_auth_passwd;
    std::string proxy_auth_token;
#ifdef CINATRA_ENABLE_SSL
    bool use_ssl = false;
    std::string base_path;
    std::string cert_file;
    int verify_mode;
    std::string domain;
#endif
  };

  coro_http_client(asio::io_context::executor_type executor)
      : socket_(std::make_shared<socket_t>(executor)),
        executor_wrapper_(executor),
        timer_(&executor_wrapper_) {}

  coro_http_client(
      coro_io::ExecutorWrapper<> *executor = coro_io::get_global_executor())
      : coro_http_client(executor->get_asio_executor()) {}

  bool init_config(const config &conf) {
    if (conf.conn_timeout_duration.has_value()) {
      set_conn_timeout(*conf.conn_timeout_duration);
    }
    if (conf.req_timeout_duration.has_value()) {
      set_req_timeout(*conf.req_timeout_duration);
    }
    if (!conf.sec_key.empty()) {
      set_ws_sec_key(conf.sec_key);
    }
    if (conf.max_single_part_size > 0) {
      set_max_single_part_size(conf.max_single_part_size);
    }
    if (!conf.proxy_host.empty()) {
      set_proxy_basic_auth(conf.proxy_host, conf.proxy_port);
    }
    if (!conf.proxy_auth_username.empty()) {
      set_proxy_basic_auth(conf.proxy_auth_username, conf.proxy_auth_passwd);
    }
    if (!conf.proxy_auth_token.empty()) {
      set_proxy_bearer_token_auth(conf.proxy_auth_token);
    }
#ifdef CINATRA_ENABLE_SSL
    if (conf.use_ssl) {
      return init_ssl(conf.base_path, conf.cert_file, conf.verify_mode,
                      conf.domain);
    }
    return true;
#endif
    return true;
  }

  ~coro_http_client() { async_close(); }

  void async_close() {
    if (socket_->has_closed_)
      return;

    asio::dispatch(executor_wrapper_.get_asio_executor(), [socket = socket_] {
      close_socket(*socket);
    });
  }

#ifdef CINATRA_ENABLE_SSL
  bool init_ssl(const std::string &base_path, const std::string &cert_file,
                int verify_mode = asio::ssl::verify_none,
                const std::string &domain = "localhost") {
    try {
      ssl_init_ret_ = false;
      ssl_ctx_ =
          std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23);
      auto full_cert_file = std::filesystem::path(base_path).append(cert_file);
      if (std::filesystem::exists(full_cert_file)) {
        ssl_ctx_->load_verify_file(full_cert_file.string());
      }
      else {
        if (!base_path.empty() || !cert_file.empty())
          return false;
      }

      ssl_ctx_->set_verify_mode(verify_mode);

      // ssl_ctx_.add_certificate_authority(asio::buffer(CA_PEM));
      if (!domain.empty())
        ssl_ctx_->set_verify_callback(
            asio::ssl::host_name_verification(domain));

      ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              socket_->impl_, *ssl_ctx_);
      // Set SNI Hostname (many hosts need this to handshake successfully)
      if (!sni_hostname_.empty()) {
        SSL_set_tlsext_host_name(ssl_stream_->native_handle(),
                                 sni_hostname_.c_str());
      }
      use_ssl_ = true;
      ssl_init_ret_ = true;
    } catch (std::exception &e) {
      std::cout << "init ssl failed: " << e.what() << "\n";
    }
    return ssl_init_ret_;
  }

  [[nodiscard]] bool init_ssl(std::string full_path = "",
                              int verify_mode = asio::ssl::verify_none,
                              const std::string &domain = "localhost") {
    std::string base_path;
    std::string cert_file;
    if (full_path.empty()) {
      base_path = "";
      cert_file = "";
    }
    else {
      base_path = full_path.substr(0, full_path.find_last_of('/'));
      cert_file = full_path.substr(full_path.find_last_of('/') + 1);
    }
    return init_ssl(base_path, cert_file, verify_mode, domain);
  }
#endif

  // return body_, the user will own body's lifetime.
  std::unique_ptr<char[]> release_buf() {
    if (body_.size() == 0) {
      return nullptr;
    }
    return std::unique_ptr<char[]>(body_.release());
  }

  // only make socket connet(or handshake) to the host
  async_simple::coro::Lazy<resp_data> connect(std::string uri) {
    resp_data data{};
    bool no_schema = !has_schema(uri);
    std::string append_uri;
    if (no_schema) {
      append_uri.append("http://").append(uri);
    }

    auto [ok, u] = handle_uri(data, no_schema ? append_uri : uri);
    if (!ok) {
      co_return resp_data{{}, 404};
    }

    auto future = start_timer(req_timeout_duration_, "connect timer");

    data = co_await connect(u);
    if (auto ec = co_await wait_timer(std::move(future)); ec) {
      co_return resp_data{{}, 404};
    }
    if (!data.net_err) {
      data.status = 200;
    }
    co_return data;
  }

  bool has_closed() { return socket_->has_closed_; }

  bool add_header(std::string key, std::string val) {
    if (key.empty())
      return false;

    if (key == "Host")
      return false;

    req_headers_[key] = std::move(val);

    return true;
  }

  void set_ws_sec_key(std::string sec_key) { ws_sec_key_ = std::move(sec_key); }

  async_simple::coro::Lazy<bool> async_ws_connect(std::string uri) {
    resp_data data{};
    auto [r, u] = handle_uri(data, uri);
    if (!r) {
#ifndef NDEBUG
      std::cout << "url error";
#endif
      co_return false;
    }

    req_context<> ctx{};
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

#ifdef BENCHMARK_TEST
  void set_bench_stop() { stop_bench_ = true; }
  void set_read_fix() { read_fix_ = 1; }
#endif

  async_simple::coro::Lazy<resp_data> async_patch(std::string uri) {
    return async_request(std::move(uri), cinatra::http_method::PATCH,
                         cinatra::req_context<>{});
  }

  async_simple::coro::Lazy<resp_data> async_options(std::string uri) {
    return async_request(std::move(uri), cinatra::http_method::OPTIONS,
                         cinatra::req_context<>{});
  }

  async_simple::coro::Lazy<resp_data> async_trace(std::string uri) {
    return async_request(std::move(uri), cinatra::http_method::TRACE,
                         cinatra::req_context<>{});
  }

  async_simple::coro::Lazy<resp_data> async_head(std::string uri) {
    return async_request(std::move(uri), cinatra::http_method::HEAD,
                         cinatra::req_context<>{});
  }

  // CONNECT example.com HTTP/1.1
  async_simple::coro::Lazy<resp_data> async_http_connect(std::string uri) {
    return async_request(std::move(uri), cinatra::http_method::CONNECT,
                         cinatra::req_context<>{});
  }

  async_simple::coro::Lazy<resp_data> async_get(std::string uri) {
    resp_data data{};
#ifdef BENCHMARK_TEST
    if (!req_str_.empty()) {
      if (has_closed()) {
        data.net_err = std::make_error_code(std::errc::not_connected);
        data.status = 404;
        co_return data;
      }

      std::error_code ec{};
      size_t size = 0;
      if (std::tie(ec, size) = co_await async_write(asio::buffer(req_str_));
          ec) {
        data.net_err = ec;
        data.status = 404;
        close_socket(*socket_);
        co_return data;
      }

      if (read_fix_ == 0) {
        req_context<> ctx{};
        bool is_keep_alive = true;
        data = co_await handle_read(ec, size, is_keep_alive, std::move(ctx),
                                    http_method::GET);
        handle_result(data, ec, is_keep_alive);
        if (ec) {
          if (!stop_bench_)
#ifndef NDEBUG
            std::cout << "do_bench_read error:" << ec.message() << "\n";
#endif
          data.net_err = ec;
          data.status = 404;
        }
        else {
          data.status = 200;
          data.total = total_len_;
        }

        co_return data;
      }

      std::tie(ec, size) = co_await async_read(read_buf_, total_len_);

      if (ec) {
        if (!stop_bench_)
#ifndef NDEBUG
          std::cout << "do_bench_read error:" << ec.message() << "\n";
#endif
        data.net_err = ec;
        data.status = 404;
        close_socket(*socket_);
        co_return data;
      }
      else {
        const char *data_ptr =
            asio::buffer_cast<const char *>(read_buf_.data());
        read_buf_.consume(total_len_);
        // check status
        if (data_ptr[9] > '3') {
          data.status = 404;
          co_return data;
        }
      }

      read_buf_.consume(total_len_);
      data.status = 200;
      data.total = total_len_;

      co_return data;
    }
#endif

    req_context<> ctx{};
    data = co_await async_request(std::move(uri), http_method::GET,
                                  std::move(ctx));
#ifdef BENCHMARK_TEST
    data.total = total_len_;
#endif
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

  resp_data post(std::string uri, std::string content,
                 req_content_type content_type) {
    return async_simple::coro::syncAwait(
        async_post(std::move(uri), std::move(content), content_type));
  }

  async_simple::coro::Lazy<resp_data> async_post(
      std::string uri, std::string content, req_content_type content_type) {
    req_context<> ctx{content_type, "", std::move(content)};
    return async_request(std::move(uri), http_method::POST, std::move(ctx));
  }

  async_simple::coro::Lazy<resp_data> async_delete(
      std::string uri, std::string content, req_content_type content_type) {
    req_context<> ctx{content_type, "", std::move(content)};
    return async_request(std::move(uri), http_method::DEL, std::move(ctx));
  }

  async_simple::coro::Lazy<resp_data> async_put(std::string uri,
                                                std::string content,
                                                req_content_type content_type) {
    req_context<> ctx{content_type, "", std::move(content)};
    return async_request(std::move(uri), http_method::PUT, std::move(ctx));
  }

  bool add_str_part(std::string name, std::string content) {
    size_t size = content.size();
    return form_data_
        .emplace(std::move(name), multipart_t{"", std::move(content), size})
        .second;
  }

  bool add_file_part(std::string name, std::string filename) {
    if (form_data_.find(name) != form_data_.end()) {
#ifndef NDEBUG
      std::cout << "name already exist\n";
#endif
      return false;
    }

    std::error_code ec;
    bool r = std::filesystem::exists(filename, ec);
    if (!r || ec) {
#ifndef NDEBUG
      if (ec) {
        std::cout << ec.message() << "\n";
      }
      std::cout << "file not exists, "
                << std::filesystem::current_path().string() << std::endl;
#endif
      return false;
    }

    size_t file_size = std::filesystem::file_size(filename);
    form_data_.emplace(std::move(name),
                       multipart_t{std::move(filename), "", file_size});
    return true;
  }

  void set_max_single_part_size(size_t size) { max_single_part_size_ = size; }

  async_simple::Future<async_simple::Unit> start_timer(
      std::chrono::steady_clock::duration duration, std::string msg) {
    is_timeout_ = false;

    async_simple::Promise<async_simple::Unit> promise;
    auto fut = promise.getFuture();

    if (enable_timeout_) {
      timeout(timer_, std::move(promise), duration, std::move(msg))
          .via(&executor_wrapper_)
          .detach();
    }
    else {
      promise.setValue(async_simple::Unit{});
    }
    return fut;
  }

  async_simple::coro::Lazy<std::error_code> wait_timer(
      async_simple::Future<async_simple::Unit> &&future) {
    if (!enable_timeout_) {
      co_return std::error_code{};
    }
    std::error_code err_code;
    timer_.cancel(err_code);
    auto ret = co_await std::move(future);
    if (is_timeout_) {
      co_return std::make_error_code(std::errc::timed_out);
    }

    co_return std::error_code{};
  }

  async_simple::coro::Lazy<resp_data> async_upload_multipart(std::string uri) {
    std::shared_ptr<int> guard(nullptr, [this](auto) {
      req_headers_.clear();
      form_data_.clear();
    });
    if (form_data_.empty()) {
#ifndef NDEBUG
      std::cout << "no multipart\n";
#endif
      co_return resp_data{{}, 404};
    }

    req_context<> ctx{req_content_type::multipart, "", ""};
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

    auto future = start_timer(req_timeout_duration_, "connect timer");

    data = co_await connect(u);
    if (ec = co_await wait_timer(std::move(future)); ec) {
      co_return resp_data{{}, 404};
    }
    if (data.net_err) {
      co_return data;
    }

    future = start_timer(req_timeout_duration_, "upload timer");
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
#ifndef NDEBUG
      std::cout << ec.message() << "\n";
#endif
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
    data = co_await handle_read(ec, size, is_keep_alive, std::move(ctx),
                                http_method::POST);
    if (auto errc = co_await wait_timer(std::move(future)); errc) {
      ec = errc;
    }

    handle_result(data, ec, is_keep_alive);
    co_return data;
  }

  async_simple::coro::Lazy<resp_data> async_upload_multipart(
      std::string uri, std::string name, std::string filename) {
    if (!add_file_part(std::move(name), std::move(filename))) {
#ifndef NDEBUG
      std::cout << "open file failed or duplicate test names\n";
#endif
      co_return resp_data{{}, 404};
    }
    co_return co_await async_upload_multipart(std::move(uri));
  }

  async_simple::coro::Lazy<resp_data> async_download(std::string uri,
                                                     std::string filename,
                                                     std::string range = "") {
    resp_data data{};
    auto file = std::make_shared<coro_io::coro_file>(filename,
                                                     coro_io::open_mode::write);
    if (!file->is_open()) {
      data.net_err = std::make_error_code(std::errc::no_such_file_or_directory);
      data.status = 404;
      co_return data;
    }

    req_context<> ctx{};
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

  void reset() {
    if (!has_closed())
      close_socket(*socket_);
    socket_->has_closed_ = true;
    socket_->impl_ = asio::ip::tcp::socket{executor_wrapper_.context()};
    if (!socket_->impl_.is_open()) {
      socket_->impl_.open(asio::ip::tcp::v4());
    }
#ifdef CINATRA_ENABLE_SSL
    sni_hostname_ = "";
#endif
#ifdef BENCHMARK_TEST
    req_str_.clear();
    total_len_ = 0;
#endif
  }

  async_simple::coro::Lazy<resp_data> reconnect(std::string uri) {
    reset();
    co_return co_await connect(std::move(uri));
  }

  template <typename S, typename String>
  async_simple::coro::Lazy<resp_data> async_upload_chunked(
      S uri, http_method method, String filename,
      std::unordered_map<std::string, std::string> headers = {}) {
    std::shared_ptr<int> guard(nullptr, [this](auto) {
      if (!req_headers_.empty()) {
        req_headers_.clear();
      }
    });

    req_context<> ctx{req_content_type::text};
    resp_data data{};
    auto [ok, u] = handle_uri(data, uri);
    if (!ok) {
      co_return resp_data{{}, 404};
    }

    if (!std::filesystem::exists(filename)) {
      co_return resp_data{
          std::make_error_code(std::errc::no_such_file_or_directory), 404};
    }

    add_header("Transfer-Encoding", "chunked");

    std::string header_str =
        build_request_header(u, method, ctx, true, std::move(headers));

    std::error_code ec{};
    size_t size = 0;

    auto future = start_timer(req_timeout_duration_, "connect timer");

    data = co_await connect(u);
    if (ec = co_await wait_timer(std::move(future)); ec) {
      co_return resp_data{{}, 404};
    }
    if (data.net_err) {
      co_return data;
    }

    future = start_timer(req_timeout_duration_, "upload timer");
    std::tie(ec, size) = co_await async_write(asio::buffer(header_str));
    if (ec) {
      co_return resp_data{ec, 404};
    }

    coro_io::coro_file file(filename, coro_io::open_mode::read);
    std::string file_data;
    file_data.resize(max_single_part_size_);
    std::string chunk_size_str;
    while (!file.eof()) {
      auto [rd_ec, rd_size] =
          co_await file.async_read(file_data.data(), file_data.size());
      auto bufs = cinatra::to_chunked_buffers<asio::const_buffer>(
          file_data.data(), rd_size, chunk_size_str, file.eof());
      if (std::tie(ec, size) = co_await async_write(bufs); ec) {
        co_return resp_data{ec, 404};
      }
    }

    bool is_keep_alive = true;
    data = co_await handle_read(ec, size, is_keep_alive, std::move(ctx),
                                http_method::POST);
    if (auto errc = co_await wait_timer(std::move(future)); errc) {
      ec = errc;
    }

    handle_result(data, ec, is_keep_alive);
    co_return data;
  }

  template <typename S, typename String>
  async_simple::coro::Lazy<resp_data> async_request(
      S uri, http_method method, req_context<String> ctx,
      std::unordered_map<std::string, std::string> headers = {}) {
    if (!resp_chunk_str_.empty()) {
      resp_chunk_str_.clear();
    }
    std::shared_ptr<int> guard(nullptr, [this](auto) {
      if (!req_headers_.empty()) {
        req_headers_.clear();
      }
    });

    resp_data data{};

    std::error_code ec{};
    size_t size = 0;
    bool is_keep_alive = true;

    do {
      uri_t u;
      std::string append_uri;

      if (socket_->has_closed_ || (!uri.empty() && uri[0] != '/')) {
        bool no_schema = !has_schema(uri);

        if (no_schema) {
          append_uri.append("http://").append(uri);
        }
        bool ok = false;
        std::tie(ok, u) = handle_uri(data, no_schema ? append_uri : uri);
        if (!ok) {
          break;
        }
      }
      else {
        u.path = uri;
      }
      if (socket_->has_closed_) {
        auto conn_future = start_timer(conn_timeout_duration_, "connect timer");
        std::string host = proxy_host_.empty() ? u.get_host() : proxy_host_;
        std::string port = proxy_port_.empty() ? u.get_port() : proxy_port_;
        if (ec = co_await coro_io::async_connect(&executor_wrapper_,
                                                 socket_->impl_, host, port);
            ec) {
          break;
        }

        socket_->impl_.set_option(asio::ip::tcp::no_delay(true));

        if (u.is_ssl) {
          if (ec = co_await handle_shake(); ec) {
            break;
          }
        }
        socket_->has_closed_ = false;
        if (ec = co_await wait_timer(std::move(conn_future)); ec) {
          break;
        }
      }

      std::vector<asio::const_buffer> vec;
      std::string req_head_str =
          build_request_header(u, method, ctx, false, std::move(headers));

      bool has_body = !ctx.content.empty();
      if (has_body) {
        vec.push_back(asio::buffer(req_head_str));
        vec.push_back(asio::buffer(ctx.content.data(), ctx.content.size()));
      }

#ifdef BENCHMARK_TEST
      req_str_ = req_head_str;
#endif
#ifdef CORO_HTTP_PRINT_REQ_HEAD
      std::cout << req_head_str << "\n";
#endif
      auto future = start_timer(req_timeout_duration_, "request timer");
      if (has_body) {
        std::tie(ec, size) = co_await async_write(vec);
      }
      else {
        std::tie(ec, size) = co_await async_write(asio::buffer(req_head_str));
      }
      if (ec) {
        break;
      }

      data =
          co_await handle_read(ec, size, is_keep_alive, std::move(ctx), method);
      if (auto errc = co_await wait_timer(std::move(future)); errc) {
        ec = errc;
      }
    } while (0);

    handle_result(data, ec, is_keep_alive);
    co_return data;
  }

  async_simple::coro::Lazy<std::error_code> handle_shake() {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      if (ssl_stream_ == nullptr) {
        co_return std::make_error_code(std::errc::not_a_stream);
      }

      auto ec = co_await coro_io::async_handshake(
          ssl_stream_, asio::ssl::stream_base::client);
      if (ec) {
#ifndef NDEBUG
        std::cout << "handle failed " << ec.message() << "\n";
#endif
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

  void set_conn_timeout(std::chrono::steady_clock::duration timeout_duration) {
    enable_timeout_ = true;
    conn_timeout_duration_ = timeout_duration;
  }

  void set_req_timeout(std::chrono::steady_clock::duration timeout_duration) {
    enable_timeout_ = true;
    req_timeout_duration_ = timeout_duration;
  }

#ifdef CINATRA_ENABLE_SSL
  void set_sni_hostname(const std::string &host) { sni_hostname_ = host; }
#endif

  template <typename T, typename U>
  friend class coro_io::client_pool;

 private:
  struct socket_t {
    asio::ip::tcp::socket impl_;
    std::atomic<bool> has_closed_ = true;
    template <typename ioc_t>
    socket_t(ioc_t &&ioc) : impl_(std::forward<ioc_t>(ioc)) {}
  };
  static bool is_ok(const resp_data &data) noexcept {
    return data.net_err == std::error_code{};
  }

  template <typename S>
  std::pair<bool, uri_t> handle_uri(resp_data &data, const S &uri) {
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

  std::string build_request_header(
      const uri_t &u, http_method method, const auto &ctx,
      bool is_chunked = false,
      std::unordered_map<std::string, std::string> headers = {}) {
    std::string req_str(method_name(method));

    req_str.append(" ").append(u.get_path());
    if (!u.query.empty()) {
      req_str.append("?").append(u.query);
    }

    if (!headers.empty()) {
      req_headers_ = std::move(headers);
      req_str.append(" HTTP/1.1\r\n");
    }
    else {
      req_str.append(" HTTP/1.1\r\nHost:").append(u.host).append("\r\n");
    }

    auto type_str = get_content_type_str(ctx.content_type);
    if (!type_str.empty()) {
      if (ctx.content_type == req_content_type::multipart) {
        type_str.append(BOUNDARY);
      }
      req_headers_["Content-Type"] = std::move(type_str);
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
      if ((method == http_method::POST || method == http_method::PUT) &&
          ctx.content_type != req_content_type::multipart) {
        should_add = true;
      }
    }

    if (is_chunked) {
      should_add = false;
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

  template <typename String>
  async_simple::coro::Lazy<resp_data> handle_read(std::error_code &ec,
                                                  size_t &size,
                                                  bool &is_keep_alive,
                                                  req_context<String> ctx,
                                                  http_method method) {
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
      if (method == http_method::HEAD) {
        co_return data;
      }

      bool is_ranges = parser.is_ranges();
      if (is_ranges) {
        is_keep_alive = true;
      }
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
#ifdef BENCHMARK_TEST
      total_len_ = parser.total_len();
#endif

      if ((size_t)parser.body_len() <= read_buf_.size()) {
        // Now get entire content, additional data will discard.
        // copy body.
        if (content_len > 0) {
          body_.init(content_len);
          auto data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
          memcpy(body_.data(), data_ptr, content_len);
          read_buf_.consume(read_buf_.size());
        }
        co_await handle_entire_content(data, content_len, is_ranges, ctx);
        break;
      }

      // read left part of content.
      size_t part_size = read_buf_.size();
      size_t size_to_read = content_len - part_size;

      body_.init(content_len);
      auto data_ptr = asio::buffer_cast<const char *>(read_buf_.data());
      memcpy(body_.data(), data_ptr, part_size);
      read_buf_.consume(part_size);

      if (std::tie(ec, size) = co_await async_read(
              asio::buffer(body_.data() + part_size, size_to_read),
              size_to_read);
          ec) {
        break;
      }

      // Now get entire content, additional data will discard.
      co_await handle_entire_content(data, content_len, is_ranges, ctx);
    } while (0);

    if (!resp_chunk_str_.empty()) {
      data.resp_body =
          std::string_view{resp_chunk_str_.data(), resp_chunk_str_.size()};
    }

    co_return data;
  }

  async_simple::coro::Lazy<void> handle_entire_content(resp_data &data,
                                                       size_t content_len,
                                                       bool is_ranges,
                                                       auto &ctx) {
    if (content_len > 0) {
      auto data_ptr = read_buf_.size() == 0
                          ? body_.data()
                          : asio::buffer_cast<const char *>(read_buf_.data());

      if (is_ranges) {
        if (ctx.stream) {
          auto ec = co_await ctx.stream->async_write(data_ptr, content_len);
          if (ec) {
            data.net_err = ec;
            co_return;
          }
        }
      }

      std::string_view reply(data_ptr, content_len);
      data.resp_body = reply;

      read_buf_.consume(content_len);
    }
    data.eof = (read_buf_.size() == 0);
  }

  void handle_result(resp_data &data, std::error_code ec, bool is_keep_alive) {
    if (ec) {
      close_socket(*socket_);
      data.net_err = ec;
      data.status = 404;
#ifdef BENCHMARK_TEST
      if (!stop_bench_) {
#ifndef NDEBUG
        std::cout << ec.message() << "\n";
#endif
      }
#endif
    }
    else {
      if (!is_keep_alive) {
        close_socket(*socket_);
      }
    }
  }

  template <typename String>
  async_simple::coro::Lazy<std::error_code> handle_chunked(
      resp_data &data, req_context<String> ctx) {
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
#ifndef NDEBUG
        std::cout << "bad chunked size\n";
#endif
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
      if (ctx.stream) {
        ec = co_await ctx.stream->async_write(data_ptr, chunk_size);
      }
      else {
        resp_chunk_str_.append(data_ptr, chunk_size);
      }

      read_buf_.consume(chunk_size + CRCF.size());
    }
    co_return ec;
  }

  async_simple::coro::Lazy<resp_data> connect(const uri_t &u) {
    if (socket_->has_closed_) {
      std::string host = proxy_host_.empty() ? u.get_host() : proxy_host_;
      std::string port = proxy_port_.empty() ? u.get_port() : proxy_port_;
      if (auto ec = co_await coro_io::async_connect(&executor_wrapper_,
                                                    socket_->impl_, host, port);
          ec) {
        co_return resp_data{ec, 404};
      }

      socket_->impl_.set_option(asio::ip::tcp::no_delay(true));

      if (u.is_ssl) {
        if (auto ec = co_await handle_shake(); ec) {
          co_return resp_data{ec, 404};
        }
      }
      socket_->has_closed_ = false;
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
      coro_io::coro_file file(part.filename, coro_io::open_mode::read);
      assert(file.is_open());
      std::string file_data;
      file_data.resize(max_single_part_size_);
      while (!file.eof()) {
        auto [rd_ec, rd_size] =
            co_await file.async_read(file_data.data(), file_data.size());
        if (auto [ec, size] =
                co_await async_write(asio::buffer(file_data.data(), rd_size));
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
        close_socket(*socket_);
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
          close_socket(*socket_);
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

        data.net_err = asio::error::eof;
        data.status = 404;
        if (on_ws_msg_)
          on_ws_msg_(data);
        co_return;
      }
      if (on_ws_msg_)
        on_ws_msg_(data);
    }
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      AsioBuffer &&buffer, size_t size_to_read) noexcept {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_read(*ssl_stream_, buffer, size_to_read);
    }
    else {
#endif
      return coro_io::async_read(socket_->impl_, buffer, size_to_read);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
      AsioBuffer &&buffer) {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_write(*ssl_stream_, buffer);
    }
    else {
#endif
      return coro_io::async_write(socket_->impl_, buffer);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_until(
      AsioBuffer &buffer, asio::string_view delim) noexcept {
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_read_until(*ssl_stream_, buffer, delim);
    }
    else {
#endif
      return coro_io::async_read_until(socket_->impl_, buffer, delim);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  static void close_socket(socket_t &socket) {
    std::error_code ec;
    socket.impl_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket.impl_.close(ec);
    socket.has_closed_ = true;
  }

  async_simple::coro::Lazy<bool> timeout(
      auto &timer, auto promise, std::chrono::steady_clock::duration duration,
      std::string msg) {
    timer.expires_after(duration);
    is_timeout_ = co_await timer.async_await();
    if (!is_timeout_) {
      promise.setValue(async_simple::Unit());
      co_return false;
    }
#ifndef NDEBUG
    std::cout << msg << " timeout\n";
#endif
    close_socket(*socket_);
    promise.setValue(async_simple::Unit());
    co_return true;
  }

  template <typename S>
  bool has_schema(const S &url) {
    size_t pos_http = url.find("http://");
    size_t pos_https = url.find("https://");
    size_t pos_ws = url.find("ws://");
    size_t pos_wss = url.find("wss://");
    bool has_http_scheme =
        ((pos_http != std::string::npos) && pos_http == 0) ||
        ((pos_https != std::string::npos) && pos_https == 0) ||
        ((pos_ws != std::string::npos) && pos_ws == 0) ||
        ((pos_wss != std::string::npos) && pos_wss == 0);
    return has_http_scheme;
  }

  coro_io::ExecutorWrapper<> executor_wrapper_;
  std::unique_ptr<asio::io_context::work> work_;
  coro_io::period_timer timer_;
  std::shared_ptr<socket_t> socket_;
  asio::streambuf read_buf_;
  simple_buffer body_{};

  std::unordered_map<std::string, std::string> req_headers_;

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
  std::unique_ptr<asio::ssl::context> ssl_ctx_ = nullptr;
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool ssl_init_ret_ = true;
  bool use_ssl_ = false;
  std::string sni_hostname_ = "";
#endif
  std::string redirect_uri_;
  bool enable_follow_redirect_ = false;

  bool is_timeout_ = false;
  bool enable_timeout_ = false;
  std::chrono::steady_clock::duration conn_timeout_duration_ =
      std::chrono::seconds(8);
  std::chrono::steady_clock::duration req_timeout_duration_ =
      std::chrono::seconds(60);
  std::string resp_chunk_str_;
#ifdef BENCHMARK_TEST
  std::string req_str_;
  bool stop_bench_ = false;
  size_t total_len_ = 0;
  int read_fix_ = 0;
#endif
};
}  // namespace cinatra
