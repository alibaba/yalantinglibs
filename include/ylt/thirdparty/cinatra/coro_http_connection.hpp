#pragma once
#include <async_simple/Executor.h>
#include <async_simple/coro/SyncAwait.h>

#include <asio/buffer.hpp>
#include <system_error>
#include <thread>

#include "asio/dispatch.hpp"
#include "asio/streambuf.hpp"
#include "async_simple/coro/Lazy.h"
#include "cinatra/cinatra_log_wrapper.hpp"
#include "cinatra/response_cv.hpp"
#include "coro_http_request.hpp"
#include "coro_http_router.hpp"
#include "define.h"
#include "http_parser.hpp"
#include "sha1.hpp"
#include "string_resize.hpp"
#include "websocket.hpp"
#include "ylt/coro_io/coro_file.hpp"
#include "ylt/coro_io/coro_io.hpp"

namespace cinatra {
struct chunked_result {
  std::error_code ec;
  bool eof = false;
  std::string_view data;
};

struct part_head_t {
  std::error_code ec;
  std::string name;
  std::string filename;
};

struct websocket_result {
  std::error_code ec;
  ws_frame_type type;
  std::string_view data;
  bool eof;
};

class coro_http_connection
    : public std::enable_shared_from_this<coro_http_connection> {
 public:
  template <typename executor_t>
  coro_http_connection(executor_t *executor, asio::ip::tcp::socket socket,
                       coro_http_router &router)
      : executor_(executor),
        socket_(std::move(socket)),
        router_(router),
        request_(parser_, this),
        response_(this) {
    buffers_.reserve(3);
  }

  ~coro_http_connection() { close(); }

#ifdef CINATRA_ENABLE_SSL
  bool init_ssl(const std::string &cert_file, const std::string &key_file,
                std::string passwd) {
    unsigned long ssl_options = asio::ssl::context::default_workarounds |
                                asio::ssl::context::no_sslv2 |
                                asio::ssl::context::single_dh_use;
    try {
      ssl_ctx_ =
          std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23);

      ssl_ctx_->set_options(ssl_options);
      if (!passwd.empty()) {
        ssl_ctx_->set_password_callback([pwd = std::move(passwd)](auto, auto) {
          return pwd;
        });
      }

      std::error_code ec;
      if (fs::exists(cert_file, ec)) {
        ssl_ctx_->use_certificate_chain_file(std::move(cert_file));
      }

      if (fs::exists(key_file, ec)) {
        ssl_ctx_->use_private_key_file(std::move(key_file),
                                       asio::ssl::context::pem);
      }

      ssl_stream_ =
          std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
              socket_, *ssl_ctx_);
      use_ssl_ = true;
    } catch (const std::exception &e) {
      CINATRA_LOG_ERROR << "init ssl failed, reason: " << e.what();
      return false;
    }
    return true;
  }
#endif

  async_simple::coro::Lazy<void> start() {
#ifdef CINATRA_ENABLE_SSL
    bool has_shake = false;
#endif
    while (true) {
#ifdef CINATRA_ENABLE_SSL
      if (use_ssl_ && !has_shake) {
        auto ec = co_await coro_io::async_handshake(
            ssl_stream_, asio::ssl::stream_base::server);
        if (ec) {
          CINATRA_LOG_ERROR << "handle_shake error: " << ec.message();
          close();
          break;
        }

        has_shake = true;
      }
#endif
      auto [ec, size] = co_await async_read_until(head_buf_, TWO_CRCF);
      if (ec) {
        if (ec != asio::error::eof) {
          CINATRA_LOG_WARNING << "read http header error: " << ec.message();
        }
        close();
        break;
      }

      const char *data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
      int head_len = parser_.parse_request(data_ptr, size, 0);
      if (head_len <= 0) {
        CINATRA_LOG_ERROR << "parse http header error";
        close();
        break;
      }

      head_buf_.consume(size);
      keep_alive_ = check_keep_alive();

      auto type = request_.get_content_type();

      if (type != content_type::chunked && type != content_type::multipart) {
        size_t body_len = parser_.body_len();
        if (body_len == 0) {
          if (parser_.method() == "GET"sv) {
            if (request_.is_upgrade()) {
              // websocket
              build_ws_handshake_head();
              bool ok = co_await reply(true);  // response ws handshake
              if (!ok) {
                close();
                break;
              }
              response_.set_delay(true);
            }
          }
        }
        else if (body_len <= head_buf_.size()) {
          if (body_len > 0) {
            detail::resize(body_, body_len);
            auto data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
            memcpy(body_.data(), data_ptr, body_len);
            head_buf_.consume(head_buf_.size());
          }
        }
        else {
          size_t part_size = head_buf_.size();
          size_t size_to_read = body_len - part_size;
          auto data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
          detail::resize(body_, body_len);
          memcpy(body_.data(), data_ptr, part_size);
          head_buf_.consume(part_size);

          auto [ec, size] = co_await async_read(
              asio::buffer(body_.data() + part_size, size_to_read),
              size_to_read);
          if (ec) {
            CINATRA_LOG_ERROR << "async_read error: " << ec.message();
            close();
            break;
          }
        }
      }

      std::string_view key = {
          parser_.method().data(),
          parser_.method().length() + 1 + parser_.url().length()};

      if (!body_.empty()) {
        request_.set_body(body_);
      }

      if (auto handler = router_.get_handler(key); handler) {
        router_.route(handler, request_, response_);
      }
      else {
        if (auto coro_handler = router_.get_coro_handler(key); coro_handler) {
          co_await router_.route_coro(coro_handler, request_, response_);
        }
        else {
          // not found
          response_.set_status(status_type::not_found);
        }
      }

      if (!response_.get_delay()) {
        co_await reply();
      }

      response_.clear();
      buffers_.clear();
      body_.clear();
    }
  }

  async_simple::coro::Lazy<bool> reply(bool need_to_bufffer = true) {
    // avoid duplicate reply
    if (need_to_bufffer) {
      response_.to_buffers(buffers_);
    }
    auto [ec, _] = co_await async_write(buffers_);
    if (ec) {
      CINATRA_LOG_ERROR << "async_write error: " << ec.message();
      close();
      co_return false;
    }

    if (!keep_alive_) {
      // now in io thread, so can close socket immediately.
      close();
    }

    co_return true;
  }

  async_simple::coro::Lazy<bool> write_data(std::string_view message) {
    std::vector<asio::const_buffer> buffers;
    buffers.push_back(asio::buffer(message));
    auto [ec, _] = co_await async_write(buffers);
    if (ec) {
      CINATRA_LOG_ERROR << "async_write error: " << ec.message();
      close();
      co_return false;
    }

    if (!keep_alive_) {
      // now in io thread, so can close socket immediately.
      close();
    }

    co_return true;
  }

  async_simple::coro::Lazy<bool> write_chunked_data(std::string_view buf,
                                                    bool eof) {
    std::string chunk_size_str = "";
    std::vector<asio::const_buffer> buffers =
        to_chunked_buffers<asio::const_buffer>(buf.data(), buf.length(),
                                               chunk_size_str, eof);
    auto [ec, _] = co_await async_write(std::move(buffers));
    if (ec) {
      CINATRA_LOG_ERROR << "async_write error: " << ec.message();
      close();
      co_return false;
    }

    if (!keep_alive_) {
      // now in io thread, so can close socket immediately.
      close();
    }

    co_return true;
  }

  bool sync_reply() { return async_simple::coro::syncAwait(reply()); }

  async_simple::coro::Lazy<bool> begin_chunked() {
    response_.set_delay(true);
    response_.set_status(status_type::ok);
    co_return co_await reply();
  }

  async_simple::coro::Lazy<bool> write_chunked(std::string_view chunked_data,
                                               bool eof = false) {
    response_.set_delay(true);
    buffers_.clear();
    response_.to_chunked_buffers(buffers_, chunked_data, eof);
    co_return co_await reply(false);
  }

  async_simple::coro::Lazy<bool> end_chunked() {
    co_return co_await write_chunked("", true);
  }

  async_simple::coro::Lazy<chunked_result> read_chunked() {
    if (head_buf_.size() > 0) {
      const char *data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
      chunked_buf_.sputn(data_ptr, head_buf_.size());
      head_buf_.consume(head_buf_.size());
    }

    chunked_result result{};
    std::error_code ec{};
    size_t size = 0;

    if (std::tie(ec, size) = co_await async_read_until(chunked_buf_, CRCF);
        ec) {
      result.ec = ec;
      close();
      co_return result;
    }

    size_t buf_size = chunked_buf_.size();
    size_t additional_size = buf_size - size;
    const char *data_ptr = asio::buffer_cast<const char *>(chunked_buf_.data());
    std::string_view size_str(data_ptr, size - CRCF.size());
    size_t chunk_size;
    auto [ptr, err] = std::from_chars(
        size_str.data(), size_str.data() + size_str.size(), chunk_size, 16);
    if (err != std::errc{}) {
      CINATRA_LOG_ERROR << "bad chunked size";
      result.ec = std::make_error_code(std::errc::invalid_argument);
      co_return result;
    }

    chunked_buf_.consume(size);

    if (chunk_size == 0) {
      // all finished, no more data
      chunked_buf_.consume(CRCF.size());
      result.eof = true;
      co_return result;
    }

    if (additional_size < size_t(chunk_size + 2)) {
      // not a complete chunk, read left chunk data.
      size_t size_to_read = chunk_size + 2 - additional_size;
      if (std::tie(ec, size) = co_await async_read(chunked_buf_, size_to_read);
          ec) {
        result.ec = ec;
        close();
        co_return result;
      }
    }

    data_ptr = asio::buffer_cast<const char *>(chunked_buf_.data());
    result.data = std::string_view{data_ptr, (size_t)chunk_size};
    chunked_buf_.consume(chunk_size + CRCF.size());

    co_return result;
  }

  async_simple::coro::Lazy<part_head_t> read_part_head() {
    if (head_buf_.size() > 0) {
      const char *data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
      chunked_buf_.sputn(data_ptr, head_buf_.size());
      head_buf_.consume(head_buf_.size());
    }

    part_head_t result{};
    std::error_code ec{};
    size_t last_size = chunked_buf_.size();
    size_t size;

    auto get_part_name = [](std::string_view data, std::string_view name,
                            size_t start) {
      start += name.length();
      size_t end = data.find("\"", start);
      return data.substr(start, end - start);
    };

    constexpr std::string_view name = "name=\"";
    constexpr std::string_view filename = "filename=\"";

    while (true) {
      if (std::tie(ec, size) = co_await async_read_until(chunked_buf_, CRCF);
          ec) {
        result.ec = ec;
        close();
        co_return result;
      }

      const char *data_ptr =
          asio::buffer_cast<const char *>(chunked_buf_.data());
      chunked_buf_.consume(size);
      if (*data_ptr == '-') {
        continue;
      }
      std::string_view data{data_ptr, size};
      if (size == 2) {  // got the head end: \r\n\r\n
        break;
      }

      if (size_t pos = data.find("name"); pos != std::string_view::npos) {
        result.name = get_part_name(data, name, pos);

        if (size_t pos = data.find("filename"); pos != std::string_view::npos) {
          result.filename = get_part_name(data, filename, pos);
        }
        continue;
      }
    }

    co_return result;
  }

  async_simple::coro::Lazy<chunked_result> read_part_body(
      std::string_view boundary) {
    chunked_result result{};
    std::error_code ec{};
    size_t size = 0;

    if (std::tie(ec, size) = co_await async_read_until(chunked_buf_, boundary);
        ec) {
      result.ec = ec;
      close();
      co_return result;
    }

    const char *data_ptr = asio::buffer_cast<const char *>(chunked_buf_.data());
    chunked_buf_.consume(size);
    result.data = std::string_view{
        data_ptr, size - boundary.size() - 4};  //-- boundary \r\n

    if (std::tie(ec, size) = co_await async_read_until(chunked_buf_, CRCF);
        ec) {
      result = {};
      result.ec = ec;
      close();
      co_return result;
    }

    data_ptr = asio::buffer_cast<const char *>(chunked_buf_.data());
    std::string data{data_ptr, size};
    if (size > 2) {
      constexpr std::string_view complete_flag = "--\r\n";
      if (data == complete_flag) {
        result.eof = true;
      }
    }

    chunked_buf_.consume(size);
    co_return result;
  }

  async_simple::coro::Lazy<std::error_code> write_websocket(
      std::string_view msg, opcode op = opcode::text) {
    auto header = ws_.format_header(msg.length(), op);
    std::vector<asio::const_buffer> buffers;
    buffers.push_back(asio::buffer(header));
    buffers.push_back(asio::buffer(msg));

    auto [ec, sz] = co_await async_write(buffers);
    co_return ec;
  }

  async_simple::coro::Lazy<websocket_result> read_websocket() {
    auto [ec, ws_hd_size] = co_await async_read(head_buf_, SHORT_HEADER);
    websocket_result result{ec};
    if (ec) {
      close();
      co_return result;
    }

    while (true) {
      const char *data_ptr = asio::buffer_cast<const char *>(head_buf_.data());
      auto status = ws_.parse_header(data_ptr, ws_.len_bytes());
      if (status == ws_header_status::complete) {
        ws_.reset_len_bytes();
        head_buf_.consume(head_buf_.size());
        std::span<char> payload{};
        auto payload_length = ws_.payload_length();
        if (payload_length > 0) {
          detail::resize(body_, payload_length);
          auto [ec, read_sz] =
              co_await async_read(asio::buffer(body_), payload_length);
          if (ec) {
            close();
            result.ec = ec;
            break;
          }
          payload = body_;
        }

        if (max_part_size_ != 0 && payload_length > max_part_size_) {
          std::string close_reason = "message_too_big";
          std::string close_msg = ws_.format_close_payload(
              close_code::too_big, close_reason.data(), close_reason.size());
          co_await write_websocket(close_msg, opcode::close);
          close();
          break;
        }

        ws_frame_type type = ws_.parse_payload(payload);

        switch (type) {
          case cinatra::ws_frame_type::WS_ERROR_FRAME:
            result.ec = std::make_error_code(std::errc::protocol_error);
            break;
          case cinatra::ws_frame_type::WS_OPENING_FRAME:
            continue;
          case ws_frame_type::WS_INCOMPLETE_TEXT_FRAME:
          case ws_frame_type::WS_INCOMPLETE_BINARY_FRAME:
            result.eof = false;
            result.data = {payload.data(), payload.size()};
            break;
          case cinatra::ws_frame_type::WS_TEXT_FRAME:
          case cinatra::ws_frame_type::WS_BINARY_FRAME: {
            result.eof = true;
            result.data = {payload.data(), payload.size()};
          } break;
          case cinatra::ws_frame_type::WS_CLOSE_FRAME: {
            close_frame close_frame =
                ws_.parse_close_payload(payload.data(), payload.size());
            result.data = {close_frame.message, close_frame.length};

            std::string close_msg = ws_.format_close_payload(
                close_code::normal, close_frame.message, close_frame.length);
            auto header = ws_.format_header(close_msg.length(), opcode::close);

            co_await write_websocket(close_msg, opcode::close);
            close();
          } break;
          case cinatra::ws_frame_type::WS_PING_FRAME: {
            auto ec = co_await write_websocket({payload.data(), payload.size()},
                                               opcode::pong);
            if (ec) {
              close();
              result.ec = ec;
            }
          } break;
          case cinatra::ws_frame_type::WS_PONG_FRAME: {
            auto ec = co_await write_websocket("ping", opcode::ping);
            close();
            result.ec = ec;
          } break;
          default:
            break;
        }

        result.type = type;
        co_return result;
      }
      else if (status == ws_header_status::incomplete) {
        auto [ec, sz] = co_await async_read(head_buf_, ws_.left_header_len());
        if (ec) {
          close();
          result.ec = ec;
          break;
        }
        continue;
      }
      else {
        close();
        result.ec = std::make_error_code(std::errc::protocol_error);
        co_return result;
      }
    }

    co_return result;
  }

  auto &tcp_socket() { return socket_; }

  void set_quit_callback(std::function<void(const uint64_t &conn_id)> callback,
                         uint64_t conn_id) {
    quit_cb_ = std::move(callback);
    conn_id_ = conn_id;
  }

  void set_ws_max_size(uint64_t max_size) { max_part_size_ = max_size; }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
      AsioBuffer &&buffer, size_t size_to_read) noexcept {
    set_last_time();
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_read(*ssl_stream_, buffer, size_to_read);
    }
    else {
#endif
      return coro_io::async_read(socket_, buffer, size_to_read);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
      AsioBuffer &&buffer) {
    set_last_time();
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_write(*ssl_stream_, buffer);
    }
    else {
#endif
      return coro_io::async_write(socket_, buffer);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  template <typename AsioBuffer>
  async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read_until(
      AsioBuffer &buffer, asio::string_view delim) noexcept {
    set_last_time();
#ifdef CINATRA_ENABLE_SSL
    if (use_ssl_) {
      return coro_io::async_read_until(*ssl_stream_, buffer, delim);
    }
    else {
#endif
      return coro_io::async_read_until(socket_, buffer, delim);
#ifdef CINATRA_ENABLE_SSL
    }
#endif
  }

  void set_last_time() {
    if (checkout_timeout_) {
      last_rwtime_ = std::chrono::system_clock::now();
    }
  }

  std::chrono::system_clock::time_point get_last_rwtime() {
    return last_rwtime_;
  }

  auto &get_executor() { return *executor_; }

  void close(bool need_cb = true) {
    if (has_closed_) {
      return;
    }

    asio::dispatch(socket_.get_executor(),
                   [this, need_cb, self = shared_from_this()] {
                     std::error_code ec;
                     socket_.shutdown(asio::socket_base::shutdown_both, ec);
                     socket_.close(ec);
                     if (need_cb && quit_cb_) {
                       quit_cb_(conn_id_);
                     }
                     has_closed_ = true;
                   });
  }

  void set_check_timeout(bool r) { checkout_timeout_ = r; }

 private:
  bool check_keep_alive() {
    bool keep_alive = true;
    auto val = request_.get_header_value("connection");
    if (!val.empty() && iequal0(val, "close")) {
      keep_alive = false;
    }

    return keep_alive;
  }

  void build_ws_handshake_head() {
    uint8_t sha1buf[20], key_src[60];
    char accept_key[29];

    std::memcpy(key_src, request_.get_header_value("sec-websocket-key").data(),
                24);
    std::memcpy(key_src + 24, ws_guid, 36);
    sha1_context ctx;
    init(ctx);
    update(ctx, key_src, sizeof(key_src));
    finish(ctx, sha1buf);

    code_utils::base64_encode(accept_key, sha1buf, sizeof(sha1buf), 0);

    response_.set_status(status_type::switching_protocols);

    response_.add_header("Upgrade", "WebSocket");
    response_.add_header("Connection", "Upgrade");
    response_.add_header("Sec-WebSocket-Accept", std::string(accept_key, 28));
    auto protocal_str = request_.get_header_value("sec-websocket-protocol");
    if (!protocal_str.empty()) {
      response_.add_header("Sec-WebSocket-Protocol", std::string(protocal_str));
    }
  }

 private:
  async_simple::Executor *executor_;
  asio::ip::tcp::socket socket_;
  coro_http_router &router_;
  asio::streambuf head_buf_;
  std::string body_;
  asio::streambuf chunked_buf_;
  http_parser parser_;
  bool keep_alive_;
  coro_http_request request_;
  coro_http_response response_;
  std::vector<asio::const_buffer> buffers_;
  std::atomic<bool> has_closed_{false};
  uint64_t conn_id_{0};
  std::function<void(const uint64_t &conn_id)> quit_cb_ = nullptr;
  bool checkout_timeout_ = false;
  std::atomic<std::chrono::system_clock::time_point> last_rwtime_;
  uint64_t max_part_size_ = 8 * 1024 * 1024;

  websocket ws_;
#ifdef CINATRA_ENABLE_SSL
  std::unique_ptr<asio::ssl::context> ssl_ctx_ = nullptr;
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
  bool use_ssl_ = false;
#endif
};
}  // namespace cinatra
