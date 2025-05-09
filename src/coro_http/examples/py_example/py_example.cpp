#include <pybind11/pybind11.h>

#include <string>

namespace py = pybind11;

std::string hello() { return "hello"; }

PYBIND11_MODULE(py_example, m) {
  m.def("hello", [] {
    return std::string("hello");
  });
}