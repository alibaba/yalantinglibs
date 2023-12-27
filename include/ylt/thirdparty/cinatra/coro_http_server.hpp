#pragma once

#include <asio/dispatch.hpp>
#include <cstdint>
#include <mutex>
#include <type_traits>

#include "asio/streambuf.hpp"
#include "async_simple/Promise.h"
#include "async_simple/coro/Lazy.h"
#include "cinatra/coro_http_response.hpp"
#include "cinatra/coro_http_router.hpp"
#include "cinatra/mime_types.hpp"
#include "cinatra_log_wrapper.hpp"
#include "coro_http_connection.hpp"
#include "ylt/coro_io/coro_file.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"

namespace cinatra {
enum class file_resp_format_type {
  chunked,
  range,
};
class coro_http_server {
 public:
  coro_http_server(asio::io_context &ctx, unsigned short port)
      : out_ctx_(&ctx), port_(port), acceptor_(ctx), check_timer_(ctx) {}

  coro_http_server(size_t thread_num, unsigned short port)
      : pool_(std::make_unique<coro_io::io_context_pool>(thread_num)),
        port_(port),
        acceptor_(pool_->get_executor()->get_asio_executor()),
        check_timer_(pool_->get_executor()->get_asio_executor()) {}

  ~coro_http_server() {
    CINATRA_LOG_INFO << "coro_http_server will quit";
    stop();
  }

  void set_no_delay(bool r) { no_delay_ = r; }

#ifdef CINATRA_ENABLE_SSL
  void init_ssl(const std::string &cert_file, const std::string &key_file,
                const std::string &passwd) {
    cert_file_ = cert_file;
    key_file_ = key_file;
    passwd_ = passwd;
    use_ssl_ = true;
  }
#endif

  // only call once, not thread safe.
  std::errc sync_start() noexcept {
    auto ret = async_start();
    ret.wait();
    return ret.value();
  }

  // only call once, not thread safe.
  async_simple::Future<std::errc> async_start() {
    auto ec = listen();

    async_simple::Promise<std::errc> promise;
    auto future = promise.getFuture();

    if (ec == std::errc{}) {
      if (out_ctx_ == nullptr) {
        thd_ = std::thread([this] {
          pool_->run();
        });
      }

      accept().start([p = std::move(promise)](auto &&res) mutable {
        if (res.hasError()) {
          p.setValue(std::errc::io_error);
        }
        else {
          p.setValue(res.value());
        }
      });
    }
    else {
      promise.setValue(ec);
    }

    return future;
  }

  // only call once, not thread safe.
  void stop() {
    if (out_ctx_ == nullptr && !thd_.joinable()) {
      return;
    }

    stop_timer_ = true;
    std::error_code ec;
    check_timer_.cancel(ec);

    close_acceptor();

    // close current connections.
    {
      std::scoped_lock lock(conn_mtx_);
      for (auto &conn : connections_) {
        conn.second->close(false);
      }
      connections_.clear();
    }

    if (out_ctx_ == nullptr) {
      CINATRA_LOG_INFO << "wait for server's thread-pool finish all work.";
      pool_->stop();

      CINATRA_LOG_INFO << "server's thread-pool finished.";
      thd_.join();
      CINATRA_LOG_INFO << "stop coro_http_server ok";
    }
    else {
      out_ctx_ = nullptr;
    }
  }

  // call it after server async_start or sync_start.
  uint16_t port() const { return port_; }

  template <http_method... method, typename Func>
  void set_http_handler(std::string key, Func handler) {
    static_assert(sizeof...(method) >= 1, "must set http_method");
    if constexpr (sizeof...(method) == 1) {
      (router_.set_http_handler<method>(std::move(key), std::move(handler)),
       ...);
    }
    else {
      (router_.set_http_handler<method>(key, handler), ...);
    }
  }

  template <http_method... method, typename Func>
  void set_http_handler(std::string key, Func handler, auto owner) {
    using return_type = typename util::function_traits<Func>::return_type;
    if constexpr (is_lazy_v<return_type>) {
      std::function<async_simple::coro::Lazy<void>(coro_http_request & req,
                                                   coro_http_response & resp)>
          f = std::bind(handler, owner, std::placeholders::_1,
                        std::placeholders::_2);
      set_http_handler<method...>(std::move(key), std::move(f));
    }
    else {
      std::function<void(coro_http_request & req, coro_http_response & resp)>
          f = std::bind(handler, owner, std::placeholders::_1,
                        std::placeholders::_2);
      set_http_handler<method...>(std::move(key), std::move(f));
    }
  }

  void set_max_size_of_cache_files(size_t max_size = 3 * 1024 * 1024) {
    std::error_code ec;
    for (const auto &file :
         std::filesystem::recursive_directory_iterator(static_dir_, ec)) {
      if (ec) {
        continue;
      }

      if (!file.is_directory()) {
        size_t filesize = fs::file_size(file, ec);
        if (ec || filesize > max_size) {
          continue;
        }

        std::ifstream ifs(file.path(), std::ios::binary);
        if (ifs.is_open()) {
          std::string content;
          detail::resize(content, filesize);
          ifs.read(content.data(), content.size());
          static_file_cache_.emplace(file.path().string(), std::move(content));
        }
      }
    }
  }

  const coro_http_router &get_router() const { return router_; }

  void set_file_resp_format_type(file_resp_format_type type) {
    format_type_ = type;
  }

  void set_transfer_chunked_size(size_t size) { chunked_size_ = size; }

  void set_static_res_dir(std::string_view uri_suffix = "",
                          std::string file_path = "www") {
    bool has_double_dot = (file_path.find("..") != std::string::npos) ||
                          (uri_suffix.find("..") != std::string::npos);
    if (std::filesystem::path(file_path).has_root_path() ||
        std::filesystem::path(uri_suffix).has_root_path() || has_double_dot) {
      CINATRA_LOG_ERROR << "invalid file path: " << file_path;
      std::exit(1);
    }

    if (!uri_suffix.empty()) {
      static_dir_router_path_ =
          std::filesystem::path(uri_suffix).make_preferred().string();
    }

    if (!file_path.empty()) {
      static_dir_ = std::filesystem::path(file_path).make_preferred().string();
    }
    else {
      static_dir_ = fs::absolute(fs::current_path().string()).string();
    }

    files_.clear();
    for (const auto &file :
         std::filesystem::recursive_directory_iterator(static_dir_)) {
      if (!file.is_directory()) {
        files_.push_back(file.path().string());
      }
    }

    std::filesystem::path router_path =
        std::filesystem::path(static_dir_router_path_);

    std::string uri;
    for (auto &file : files_) {
      auto relative_path =
          std::filesystem::path(file.substr(static_dir_.length())).string();
      if (size_t pos = relative_path.find('\\') != std::string::npos) {
        replace_all(relative_path, "\\", "/");
      }

      if (static_dir_router_path_.empty()) {
        uri = relative_path;
      }
      else {
        uri = fs::path("/")
                  .append(static_dir_router_path_)
                  .concat(relative_path)
                  .string();
      }

      set_http_handler<cinatra::GET>(
          uri,
          [this, file_name = file](
              coro_http_request &req,
              coro_http_response &resp) -> async_simple::coro::Lazy<void> {
            std::string_view extension = get_extension(file_name);
            std::string_view mime = get_mime_type(extension);

            if (auto it = static_file_cache_.find(file_name);
                it != static_file_cache_.end()) {
              auto range_header =
                  build_range_header(mime, file_name, fs::file_size(file_name));
              resp.set_delay(true);
              std::string &body = it->second;
              std::array<asio::const_buffer, 2> arr{asio::buffer(range_header),
                                                    asio::buffer(body)};
              co_await req.get_conn()->async_write(arr);
              co_return;
            }

            std::string content;
            detail::resize(content, chunked_size_);

            coro_io::coro_file in_file{};
            co_await in_file.async_open(file_name, coro_io::flags::read_only);
            if (!in_file.is_open()) {
              resp.set_status_and_content(status_type::not_found,
                                          file_name + "not found");
              co_return;
            }

            if (format_type_ == file_resp_format_type::chunked) {
              resp.set_format_type(format_type::chunked);
              bool ok;
              if (ok = co_await resp.get_conn()->begin_chunked(); !ok) {
                co_return;
              }

              while (true) {
                auto [ec, size] =
                    co_await in_file.async_read(content.data(), content.size());
                if (ec) {
                  resp.set_status(status_type::no_content);
                  co_await resp.get_conn()->reply();
                  co_return;
                }

                bool r = co_await resp.get_conn()->write_chunked(
                    std::string_view(content.data(), size));
                if (!r) {
                  co_return;
                }

                if (in_file.eof()) {
                  co_await resp.get_conn()->end_chunked();
                  break;
                }
              }
            }
            else {
              auto range_header = build_range_header(
                  mime, file_name, coro_io::coro_file::file_size(file_name));
              resp.set_delay(true);
              bool r = co_await req.get_conn()->write_data(range_header);
              if (!r) {
                co_return;
              }

              while (true) {
                auto [ec, size] =
                    co_await in_file.async_read(content.data(), content.size());
                if (ec) {
                  resp.set_status(status_type::no_content);
                  co_await resp.get_conn()->reply();
                  co_return;
                }

                r = co_await req.get_conn()->write_data(
                    std::string_view(content.data(), size));
                if (!r) {
                  co_return;
                }

                if (in_file.eof()) {
                  break;
                }
              }
            }
          });
    }
  }

  void set_check_duration(auto duration) { check_duration_ = duration; }

  void set_timeout_duration(
      std::chrono::steady_clock::duration timeout_duration) {
    if (timeout_duration > std::chrono::steady_clock::duration::zero()) {
      need_check_ = true;
      timeout_duration_ = timeout_duration;
      start_check_timer();
    }
  }

  size_t connection_count() {
    std::scoped_lock lock(conn_mtx_);
    return connections_.size();
  }

 private:
  std::errc listen() {
    CINATRA_LOG_INFO << "begin to listen";
    using asio::ip::tcp;
    auto endpoint = tcp::endpoint(tcp::v4(), port_);
    acceptor_.open(endpoint.protocol());
#ifdef __GNUC__
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    asio::error_code ec;
    acceptor_.bind(endpoint, ec);
    if (ec) {
      CINATRA_LOG_ERROR << "bind port: " << port_ << " error: " << ec.message();
      acceptor_.cancel(ec);
      acceptor_.close(ec);
      return std::errc::address_in_use;
    }
#ifdef _MSC_VER
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_.listen();

    auto end_point = acceptor_.local_endpoint(ec);
    if (ec) {
      CINATRA_LOG_ERROR << "get local endpoint port: " << port_
                        << " error: " << ec.message();
      return std::errc::address_in_use;
    }
    port_ = end_point.port();

    CINATRA_LOG_INFO << "listen port " << port_ << " successfully";
    return {};
  }

  async_simple::coro::Lazy<std::errc> accept() {
    for (;;) {
      coro_io::ExecutorWrapper<> *executor;
      if (out_ctx_ == nullptr) {
        executor = pool_->get_executor();
      }
      else {
        out_executor_ = std::make_unique<coro_io::ExecutorWrapper<>>(
            out_ctx_->get_executor());
        executor = out_executor_.get();
      }

      asio::ip::tcp::socket socket(executor->get_asio_executor());
      auto error = co_await coro_io::async_accept(acceptor_, socket);
      if (error) {
        CINATRA_LOG_INFO << "accept failed, error: " << error.message();
        if (error == asio::error::operation_aborted ||
            error == asio::error::bad_descriptor) {
          acceptor_close_waiter_.set_value();
          co_return std::errc::operation_canceled;
        }
        continue;
      }

      uint64_t conn_id = ++conn_id_;
      CINATRA_LOG_DEBUG << "new connection comming, id: " << conn_id;
      auto conn = std::make_shared<coro_http_connection>(
          executor, std::move(socket), router_);
      if (no_delay_) {
        conn->tcp_socket().set_option(asio::ip::tcp::no_delay(true));
      }
      if (need_check_) {
        conn->set_check_timeout(true);
      }

#ifdef CINATRA_ENABLE_SSL
      if (use_ssl_) {
        conn->init_ssl(cert_file_, key_file_, passwd_);
      }
#endif

      conn->set_quit_callback(
          [this](const uint64_t &id) {
            std::scoped_lock lock(conn_mtx_);
            if (!connections_.empty())
              connections_.erase(id);
          },
          conn_id);

      {
        std::scoped_lock lock(conn_mtx_);
        connections_.emplace(conn_id, conn);
      }

      start_one(conn).via(&conn->get_executor()).detach();
    }
  }

  async_simple::coro::Lazy<void> start_one(
      std::shared_ptr<coro_http_connection> conn) noexcept {
    co_await conn->start();
  }

  void close_acceptor() {
    asio::dispatch(acceptor_.get_executor(), [this]() {
      asio::error_code ec;
      acceptor_.cancel(ec);
      acceptor_.close(ec);
    });
    acceptor_close_waiter_.get_future().wait();
  }

  void start_check_timer() {
    check_timer_.expires_after(check_duration_);
    check_timer_.async_wait([this](auto ec) {
      if (ec || stop_timer_) {
        return;
      }

      check_timeout();
      start_check_timer();
    });
  }

  void check_timeout() {
    auto cur_time = std::chrono::system_clock::now();

    std::unordered_map<uint64_t, std::shared_ptr<coro_http_connection>> conns;

    {
      std::scoped_lock lock(conn_mtx_);
      for (auto it = connections_.begin();
           it != connections_.end();)  // no "++"!
      {
        if (cur_time - it->second->get_last_rwtime() > timeout_duration_) {
          it->second->close(false);
          connections_.erase(it++);
        }
        else {
          ++it;
        }
      }
    }
  }

  std::string build_range_header(std::string_view mime,
                                 std::string_view filename, size_t file_size) {
    std::string header_str =
        "HTTP/1.1 200 OK\r\nAccess-Control-Allow-origin: "
        "*\r\nAccept-Ranges: bytes\r\n";
    header_str.append("Content-Disposition: attachment;filename=");
    header_str.append(filename).append("\r\n");
    header_str.append("Connection: keep-alive\r\n");
    header_str.append("Content-Type: ").append(mime).append("\r\n");
    header_str.append("Content-Length: ");
    header_str.append(std::to_string(file_size)).append("\r\n\r\n");
    return header_str;
  }

 private:
  std::unique_ptr<coro_io::io_context_pool> pool_;
  asio::io_context *out_ctx_ = nullptr;
  std::unique_ptr<coro_io::ExecutorWrapper<>> out_executor_ = nullptr;
  uint16_t port_;
  asio::ip::tcp::acceptor acceptor_;
  std::thread thd_;
  std::promise<void> acceptor_close_waiter_;
  bool no_delay_ = true;

  uint64_t conn_id_ = 0;
  std::unordered_map<uint64_t, std::shared_ptr<coro_http_connection>>
      connections_;
  std::mutex conn_mtx_;
  std::chrono::steady_clock::duration check_duration_ =
      std::chrono::seconds(15);
  std::chrono::steady_clock::duration timeout_duration_{};
  asio::steady_timer check_timer_;
  bool need_check_ = false;
  std::atomic<bool> stop_timer_ = false;

  std::string static_dir_router_path_ = "";
  std::string static_dir_ = "";
  std::vector<std::string> files_;
  size_t chunked_size_ = 1024 * 10;

  std::unordered_map<std::string, std::string> static_file_cache_;
  file_resp_format_type format_type_ = file_resp_format_type::chunked;
#ifdef CINATRA_ENABLE_SSL
  std::string cert_file_;
  std::string key_file_;
  std::string passwd_;
  bool use_ssl_ = false;
#endif
  coro_http_router router_;
};
}  // namespace cinatra