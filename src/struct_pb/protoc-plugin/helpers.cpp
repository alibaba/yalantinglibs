#include "helpers.hpp"

#include <algorithm>
#include <sstream>

#include "google/protobuf/io/printer.h"
namespace struct_pb {
namespace compiler {
// https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
template <typename Out>
void split(const std::string &s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

static std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}
NamespaceOpener::NamespaceOpener(google::protobuf::io::Printer *p,
                                 const std::string &package)
    : p_(p) {
  ns_ = split(package, '.');
}
NamespaceOpener::~NamespaceOpener() {}
void NamespaceOpener::open() const {
  for (const auto &ns : ns_) {
    p_->Print({{"ns", ns}}, R"(
namespace $ns$ {
)");
  }
}
void NamespaceOpener::close() const {
  for (int i = 0; i < ns_.size(); ++i) {
    auto ns = ns_[ns_.size() - i - 1];
    p_->Print({{"ns", ns}}, R"(
} // namespace $ns$
)");
  }
}
// from std::string_view ends_with
bool string_ends_with(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), std::string_view::npos, suffix) ==
             0;
}
std::string_view string_strip_suffix(const std::string_view s,
                                     const std::string_view suffix) {
  if (string_ends_with(s, suffix)) {
    return s.substr(0, s.size() - suffix.size());
  }
  return s;
}
std::string strip_proto(const std::string &filename) {
  if (string_ends_with(filename, ".protodevel")) {
    return std::string(string_strip_suffix(filename, ".protodevel"));
  }
  else {
    return std::string(string_strip_suffix(filename, ".proto"));
  }
}
}  // namespace compiler
}  // namespace struct_pb