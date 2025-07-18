#include <pybind11/pybind11.h>

#include <string>

namespace py = pybind11;

PYBIND11_MODULE(py_coro_rpc, m) {
  m.def("hello", [] {
    return std::string("hello");
  });
}