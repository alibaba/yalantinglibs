#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <chrono>
#include <rest_rpc.hpp>
#include <string>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

namespace py = pybind11;

class py_coro_rpc_server {
 public:
  py_coro_rpc_server(size_t thd_num, std::string address, size_t seconds = 0)
      : server_(thd_num, address, std::chrono::seconds(seconds)) {}

  bool start() {
    auto ec = server_.start();
    return ec.val() == 0;
  }

  bool async_start() {
    auto ec = server_.async_start();
    return !ec.hasResult();
  }

  void stop() { server_.stop(); }

  void register_handler(std::string name, py::function func) {}

 private:
  coro_rpc::coro_rpc_server server_;
};

class py_rest_conn {
 public:
  int64_t conn_id() { return conn_->conn_id(); }
  rest_rpc::rpc_service::connection* conn_;
};

class py_rest_rpc_server {
 public:
  py_rest_rpc_server(size_t port, size_t thd_num) : server_(port, thd_num) {}

  void async_start() { server_.async_run(); }

  void start() { server_.run(); }

  void register_handler(std::string name, py::function func) {
    server_.register_handler(name,
                             [func](rest_rpc::rpc_service::rpc_conn conn,
                                    std::string arg) -> std::string {
                               py::gil_scoped_acquire acquire;
                               auto sp = conn.lock();
                               py_rest_conn t{};
                               t.conn_ = sp.get();
                               auto result = func(t, arg);
                               return result.cast<std::string>();
                             });
  }

 private:
  rest_rpc::rpc_service::rpc_server server_;
};

class py_rest_rpc_client {
 public:
  py_rest_rpc_client(std::string host, uint16_t port) : client_(host, port) {}

  bool connect() { return client_.connect(); }

  std::string call(std::string func_name, std::string arg) {
    return client_.call<std::string>(func_name, arg);
  }

 private:
  rest_rpc::rpc_client client_;
};

PYBIND11_MODULE(py_coro_rpc, m) {
  m.def("hello", [] {
    return std::string("hello");
  });

  py::class_<py_coro_rpc_server>(m, "coro_rpc_server")
      .def(py::init<size_t, std::string, size_t>())
      .def("start", &py_coro_rpc_server::start)
      .def("async_start", &py_coro_rpc_server::async_start)
      .def("stop", &py_coro_rpc_server::stop);

  py::class_<py_rest_rpc_server>(m, "rest_rpc_server")
      .def(py::init<size_t, size_t>())
      .def("start", &py_rest_rpc_server::start)
      .def("async_start", &py_rest_rpc_server::async_start)
      .def("register_handler", &py_rest_rpc_server::register_handler);

  py::class_<py_rest_rpc_client>(m, "rest_rpc_client")
      .def(py::init<std::string, uint16_t>())
      .def("connect", &py_rest_rpc_client::connect)
      .def("call", &py_rest_rpc_client::call,
           py::call_guard<py::gil_scoped_release>());

  py::class_<py_rest_conn>(m, "py_rest_conn")
      .def(py::init<>())
      .def("conn_id", &py_rest_conn::conn_id);
}