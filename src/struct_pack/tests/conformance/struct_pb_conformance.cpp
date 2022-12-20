#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "conformance.struct_pb.hpp"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/type_resolver.h"
using namespace google::protobuf;
namespace struct_pack::pb::conformance {

std::errc read_fd(int fd, char* buf, std::size_t len) {
  while (len > 0) {
    ssize_t bytes_read = read(fd, buf, len);
    if (bytes_read == 0) {
      std::cout << "EOF" << std::endl;
      return std::errc::io_error;
    }
    if (bytes_read < 0) {
      std::cout << "read error" << std::endl;
      return std::errc::io_error;
    }
    len -= bytes_read;
    buf += bytes_read;
  }
  return std::errc{};
}
std::errc write_fd(int fd, const void* buf, std::size_t len) {
  if (static_cast<std::size_t>(write(fd, buf, len) != len)) {
    std::cout << "write error" << std::endl;
    return std::errc::io_error;
  }
  return std::errc{};
}
class harness {
  std::pair<std::errc, ConformanceResponse> run_test(
      const ConformanceRequest& request) {
    std::cout << request.message_type << std::endl;
    auto descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(
        request.message_type);
    //    if (descriptor == nullptr) {
    //      std::cout << "No such message type: " << request.message_type <<
    //      std::endl; return {
    //        std::errc::no_message, {}
    //      };
    //    }
    std::unique_ptr<Message> test_message(
        MessageFactory::generated_factory()->GetPrototype(descriptor)->New());
    //    ConformanceResponse response;
    //    auto idx = request.payload.index();
    //    switch (idx) {
    //      case 0: {
    //        auto buffer = std::get<0>(request.payload);
    //      }
    //      default: {
    //
    //      }
    //    }
    return {};
  }
  std::pair<std::errc, bool> serve_conformance_request() {}

 private:
  bool verbose_ = true;
  std::unique_ptr<google::protobuf::util::TypeResolver> resolver_;
  std::string type_url_;
};
}  // namespace struct_pack::pb::conformance
int main() { return 0; }