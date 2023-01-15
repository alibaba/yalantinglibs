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

}  // namespace compiler
}  // namespace struct_pb