#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

namespace py = pybind11;

class py_rpc_context {
 public:
  void response_msg(py::buffer msg, py::handle done) {
    py::buffer_info info = msg.request();
    const char *data = static_cast<char *>(info.ptr);
    context_.get_context_info()->set_response_attachment(
        std::string_view(data, info.size));
    done.inc_ref();
    context_.get_context_info()->set_complete_handler(
        [done](const std::error_code &ec, std::size_t) {
          py::gil_scoped_acquire acquire;
          done(!ec);
          done.dec_ref();
        });
    context_.response_msg();
  }

  coro_rpc::context<void> context_;
};

class py_coro_rpc_client_pool;
class py_coro_rpc_server {
 public:
  py_coro_rpc_server(size_t thd_num, std::string address,
                     py::handle py_callback, size_t seconds)
      : server_(thd_num, address, std::chrono::seconds(seconds)),
        py_callback_(py_callback) {
    server_.register_handler<&py_coro_rpc_server::handle_msg>(this);
  }

  bool start() {
    auto ec = server_.start();
    return ec.val() == 0;
  }

  bool async_start() {
    auto ec = server_.async_start();
    return !ec.hasResult();
  }

 private:
  friend class py_coro_rpc_client_pool;
  void handle_msg(coro_rpc::context<void> context, std::string_view msg) {
    py_rpc_context t{};
    t.context_ = std::move(context);
    py::gil_scoped_acquire acquire;
    auto view = py::memoryview::from_buffer(msg.data(), {msg.size()},
                                            {sizeof(uint8_t)});
    py_callback_(std::move(t), view);
  }

  coro_rpc::coro_rpc_server server_;
  py::handle py_callback_;
};

class py_coro_rpc_client_pool {
 public:
  py_coro_rpc_client_pool(std::string url)
      : pool_(coro_io::client_pool<coro_rpc::coro_rpc_client>::create(url)){};

  pybind11::object async_send_msg(py::handle loop, py::handle py_bytes) {
    auto local_future = loop.attr("create_future")();
    py::handle future = local_future;

    py_bytes.inc_ref();

    pool_
        ->send_request([py_bytes, loop,
                        future](coro_rpc::coro_rpc_client &client)
                           -> async_simple::coro::Lazy<void> {
          char *data;
          ssize_t length;
          PyBytes_AsStringAndSize(py_bytes.ptr(), &data, &length);
          auto result = co_await client.call<&py_coro_rpc_server::handle_msg>(
              std::string_view(data, length));
          py::gil_scoped_acquire acquire;
          loop.attr("call_soon_threadsafe")(future.attr("set_result"),
                                            result.has_value());
          py_bytes.dec_ref();
        })
        .start([](auto &&) {
        });

    return local_future;
  }

 private:
  std::shared_ptr<coro_io::client_pool<coro_rpc::coro_rpc_client>> pool_;
};

PYBIND11_MODULE(py_coro_rpc, m) {
  m.def("hello", [] {
    return std::string("hello");
  });

  py::class_<py_rpc_context>(m, "py_rpc_context")
      .def(py::init<>())
      .def("response_msg", &py_rpc_context::response_msg);

  py::class_<py_coro_rpc_server>(m, "coro_rpc_server")
      .def(py::init<size_t, std::string, py::function, size_t>())
      .def("start", &py_coro_rpc_server::start)
      .def("async_start", &py_coro_rpc_server::async_start);

  py::class_<py_coro_rpc_client_pool>(m, "py_coro_rpc_client_pool")
      .def(py::init<std::string>())
      .def("async_send_msg", &py_coro_rpc_client_pool::async_send_msg);

  m.def("log", [](std::string str) {
    ELOG_INFO << str;
  });
}