#include <pybind11/pybind11.h>

#include <string>
#include <ylt/coro_http/coro_http_client.hpp>
#include <ylt/coro_io/client_pool.hpp>

namespace py = pybind11;

class caller {
 public:
  caller(py::function callback) : callback_(std::move(callback)) {}
  void async_get(std::string url) {
    static auto pool =
        coro_io::client_pool<coro_http::coro_http_client>::create(url);
    pool->send_request([this, url](coro_http::coro_http_client &client)
                           -> async_simple::coro::Lazy<void> {
          auto result = co_await client.async_get(url);

          // PyObject *py_str = PyUnicode_FromStringAndSize(
          //     result.resp_body.data(), result.resp_body.size()); // zero copy
          // py::gil_scoped_acquire acquire;
          // callback_(py::reinterpret_steal<py::object>(py_str));

          py::gil_scoped_acquire acquire;
          callback_(result.resp_body);
        })
        .start([](auto &&) {
        });
  }

 private:
  py::function callback_;
};

PYBIND11_MODULE(py_example, m) {
  m.def("hello", [] {
    return std::string("hello");
  });

  py::class_<caller>(m, "caller")
      .def(py::init<py::function>())
      .def("async_get", &caller::async_get,
           py::call_guard<py::gil_scoped_release>());
}