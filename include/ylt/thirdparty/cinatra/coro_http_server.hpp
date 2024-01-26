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
  void set_http_handler(
      std::string key, Func handler,
      std::vector<std::shared_ptr<base_aspect>> aspects = {}) {
    static_assert(sizeof...(method) >= 1, "must set http_method");
    if constexpr (sizeof...(method) == 1) {
      (router_.set_http_handler<method>(std::move(key), std::move(handler),
                                        std::move(aspects)),
       ...);
    }
    else {
      (router_.set_http_handler<method>(key, handler, aspects), ...);
    }
  }

  template <http_method... method, typename Func, typename Owner>
  void set_http_handler(
      std::string key, Func handler, Owner &&owner,
      std::vector<std::shared_ptr<base_aspect>> aspects = {}) {
    static_assert(std::is_member_function_pointer_v<Func>,
                  "must be member function");
    using return_type = typename util::function_traits<Func>::return_type;
    if constexpr (is_lazy_v<return_type>) {
      std::function<async_simple::coro::Lazy<void>(coro_http_request & req,
                                                   coro_http_response & resp)>
          f = std::bind(handler, owner, std::placeholders::_1,
                        std::placeholders::_2);
      set_http_handler<method...>(std::move(key), std::move(f),
                                  std::move(aspects));
    }
    else {
      std::function<void(coro_http_request & req, coro_http_response & resp)>
          f = std::bind(handler, owner, std::placeholders::_1,
                        std::placeholders::_2);
      set_http_handler<method...>(std::move(key), std::move(f),
                                  std::move(aspects));
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

  void set_static_res_dir(
      std::string_view uri_suffix = "", std::string file_path = "www",
      std::vector<std::shared_ptr<base_aspect>> aspects = {}) {
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
            auto range_str = req.get_header_value("Range");

            if (auto it = static_file_cache_.find(file_name);
                it != static_file_cache_.end()) {
              auto range_header = build_range_header(
                  mime, file_name, std::to_string(fs::file_size(file_name)));
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

            size_t file_size = fs::file_size(file_name);

            if (format_type_ == file_resp_format_type::chunked &&
                range_str.empty()) {
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
              auto pos = range_str.find('=');
              if (pos != std::string_view::npos) {
                range_str = range_str.substr(pos + 1);
                bool is_valid = true;
                auto ranges =
                    parse_ranges(range_str, fs::file_size(file_name), is_valid);
                if (!is_valid) {
                  resp.set_status(status_type::range_not_satisfiable);
                  co_return;
                }

                assert(!ranges.empty());

                if (ranges.size() == 1) {
                  // single part
                  auto [start, end] = ranges[0];
                  in_file.seek(start, SEEK_SET);
                  size_t part_size = end + 1 - start;
                  int status = (part_size == file_size) ? 200 : 206;
                  std::string content_range = "Content-Range: bytes ";
                  content_range.append(std::to_string(start))
                      .append("-")
                      .append(std::to_string(end))
                      .append("/")
                      .append(std::to_string(file_size))
                      .append(CRCF);
                  auto range_header = build_range_header(
                      mime, file_name, std::to_string(part_size), status,
                      content_range);
                  resp.set_delay(true);
                  bool r = co_await req.get_conn()->write_data(range_header);
                  if (!r) {
                    co_return;
                  }

                  co_await send_single_part(in_file, content, req, resp,
                                            part_size);
                }
                else {
                  // multipart ranges
                  resp.set_delay(true);
                  std::string file_size_str = std::to_string(file_size);
                  size_t content_len = 0;
                  std::vector<std::string> multi_heads = build_part_heads(
                      ranges, mime, file_size_str, content_len);
                  auto range_header = build_multiple_range_header(content_len);
                  bool r = co_await req.get_conn()->write_data(range_header);
                  if (!r) {
                    co_return;
                  }

                  for (int i = 0; i < ranges.size(); i++) {
                    std::string &part_header = multi_heads[i];
                    r = co_await req.get_conn()->write_data(part_header);
                    if (!r) {
                      co_return;
                    }

                    auto [start, end] = ranges[i];
                    in_file.seek(start, SEEK_SET);
                    size_t part_size = end + 1 - start;

                    std::string_view more = CRCF;
                    if (i == ranges.size() - 1) {
                      more = MULTIPART_END;
                    }
                    r = co_await send_single_part(in_file, content, req, resp,
                                                  part_size, more);
                    if (!r) {
                      co_return;
                    }
                  }
                }
                co_return;
              }

              auto range_header = build_range_header(mime, file_name,
                                                     std::to_string(file_size));
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
          },
          aspects);
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

  void set_shrink_to_fit(bool r) { need_shrink_every_time_ = r; }

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
      if (need_shrink_every_time_) {
        conn->set_shrink_to_fit(true);
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

  std::string build_multiple_range_header(size_t content_len) {
    std::string header_str = "HTTP/1.1 206 Partial Content\r\n";
    header_str.append("Content-Length: ");
    header_str.append(std::to_string(content_len)).append(CRCF);
    header_str.append("Content-Type: multipart/byteranges; boundary=");
    header_str.append(BOUNDARY).append(TWO_CRCF);
    return header_str;
  }

  std::vector<std::string> build_part_heads(auto &ranges, std::string_view mime,
                                            std::string_view file_size_str,
                                            size_t &content_len) {
    std::vector<std::string> multi_heads;
    for (auto [start, end] : ranges) {
      std::string part_header = "--";
      part_header.append(BOUNDARY).append(CRCF);
      part_header.append("Content-Type: ").append(mime).append(CRCF);
      part_header.append("Content-Range: ").append("bytes ");
      part_header.append(std::to_string(start))
          .append("-")
          .append(std::to_string(end))
          .append("/")
          .append(file_size_str)
          .append(TWO_CRCF);
      content_len += part_header.size();
      multi_heads.push_back(std::move(part_header));
      size_t part_size = end + 1 - start + CRCF.size();
      content_len += part_size;
    }
    content_len += (BOUNDARY.size() + 4);
    return multi_heads;
  }

  std::string build_range_header(std::string_view mime,
                                 std::string_view filename,
                                 std::string_view file_size_str,
                                 int status = 200,
                                 std::string_view content_range = "") {
    std::string header_str = "HTTP/1.1 ";
    header_str.append(std::to_string(status));
    header_str.append(
        " OK\r\nAccess-Control-Allow-origin: "
        "*\r\nAccept-Ranges: bytes\r\n");
    if (!content_range.empty()) {
      header_str.append(content_range);
    }
    header_str.append("Content-Disposition: attachment;filename=");
    header_str.append(filename).append("\r\n");
    header_str.append("Connection: keep-alive\r\n");
    header_str.append("Content-Type: ").append(mime).append("\r\n");
    header_str.append("Content-Length: ");
    header_str.append(file_size_str).append("\r\n\r\n");
    return header_str;
  }

  async_simple::coro::Lazy<bool> send_single_part(auto &in_file, auto &content,
                                                  auto &req, auto &resp,
                                                  size_t part_size,
                                                  std::string_view more = "") {
    while (true) {
      size_t read_size = (std::min)(part_size, chunked_size_);
      if (read_size == 0) {
        break;
      }
      auto [ec, size] = co_await in_file.async_read(content.data(), read_size);
      if (ec) {
        resp.set_status(status_type::no_content);
        co_await resp.get_conn()->reply();
        co_return false;
      }

      part_size -= read_size;

      bool r = true;
      if (more.empty()) {
        r = co_await req.get_conn()->write_data(
            std::string_view(content.data(), size));
      }
      else {
        std::array<asio::const_buffer, 2> arr{
            asio::buffer(content.data(), size), asio::buffer(more)};
        auto [ec, _] = co_await req.get_conn()->async_write(arr);
        if (ec) {
          r = false;
        }
      }

      if (!r) {
        co_return false;
      }
    }

    co_return true;
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
  bool need_shrink_every_time_ = false;
};

using http_server = coro_http_server;
using request = coro_http_request;
using response = coro_http_response;
}  // namespace cinatra