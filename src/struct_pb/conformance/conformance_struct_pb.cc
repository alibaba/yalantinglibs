#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
namespace fs = std::filesystem;

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/conformance.struct_pb.h"
#include "google/protobuf/test_messages_proto2.struct_pb.h"
#include "google/protobuf/test_messages_proto3.struct_pb.h"

#define RETURN_IF_ERROR(expr)                                                \
  do {                                                                       \
    /* Using _status below to avoid capture problems if expr is "status". */ \
    const absl::Status _status = (expr);                                     \
    if (!_status.ok())                                                       \
      return _status;                                                        \
  } while (0)

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;

absl::Status ReadFd(int fd, char* buf, size_t len) {
  while (len > 0) {
    ssize_t bytes_read = read(fd, buf, len);

    if (bytes_read == 0) {
      return absl::DataLossError("unexpected EOF");
    }

    if (bytes_read < 0) {
      return absl::ErrnoToStatus(errno, "error reading from test runner");
    }

    len -= bytes_read;
    buf += bytes_read;
  }
  return absl::OkStatus();
}

absl::Status WriteFd(int fd, const void* buf, size_t len) {
  if (static_cast<size_t>(write(fd, buf, len)) != len) {
    return absl::ErrnoToStatus(errno, "error reading to test runner");
  }
  return absl::OkStatus();
}

class Harness {
 public:
  Harness() {}

  absl::StatusOr<ConformanceResponse> RunTest(
      const ConformanceRequest& request);

  // Returns Ok(true) if we're done processing requests.
  absl::StatusOr<bool> ServeConformanceRequest();

 private:
  bool verbose_ = false;
  std::string type_url_;
};

template <typename T>
std::string serialize(const T& t,
                      const struct_pb::UnknownFields& unknown_fields = {}) {
  std::string buffer;
  auto sz = struct_pb::internal::get_needed_size(t, unknown_fields);
  buffer.resize(sz);
  struct_pb::internal::serialize_to(buffer.data(), sz, t, unknown_fields);
  return buffer;
}

template <typename T>
absl::StatusOr<ConformanceResponse> run_test(
    const ConformanceRequest& request) {
  ConformanceResponse response{};
  T data{};
  struct_pb::UnknownFields unknown_fields{};
  switch (request.payload_case()) {
    case ConformanceRequest::PayloadCase::protobuf_payload: {
      const auto& buffer = request.protobuf_payload();
      bool ok = struct_pb::internal::deserialize_to(
          data, buffer.data(), buffer.size(), unknown_fields);
      if (!ok) {
        response.set_parse_error("parse error (no more details available)");
        return response;
      }
      break;
    }
    case ConformanceRequest::PayloadCase::none:
      return absl::UnimplementedError("payload none");
    case ConformanceRequest::PayloadCase::json_payload:
      return absl::UnimplementedError("payload json");
    case ConformanceRequest::PayloadCase::jspb_payload:
      return absl::UnimplementedError("payload jspb");
    case ConformanceRequest::PayloadCase::text_payload:
      return absl::UnimplementedError("payload text");
    default: {
      int num = static_cast<int>(request.payload_case());
      return absl::UnimplementedError("unknown payload type: " +
                                      std::to_string(num));
    }
  }
  switch (request.requested_output_format) {
    case conformance::WireFormat::PROTOBUF: {
      response.set_protobuf_payload(serialize(data, unknown_fields));
      break;
    }
    case conformance::WireFormat::UNSPECIFIED:
      return absl::InvalidArgumentError("unspecified output format");
    case conformance::WireFormat::JSON:
      return absl::UnimplementedError("output json");
    case conformance::WireFormat::JSPB:
      return absl::UnimplementedError("output jspb");
    case conformance::WireFormat::TEXT_FORMAT:
      return absl::UnimplementedError("output text");
    default: {
      int num = static_cast<int>(request.requested_output_format);
      return absl::UnimplementedError("unknown output type: " +
                                      std::to_string(num));
    }
  }
  return response;
}

absl::StatusOr<ConformanceResponse> Harness::RunTest(
    const ConformanceRequest& request) {
  std::string pb2 = "protobuf_test_messages.proto2.TestAllTypesProto2";
  std::string pb3 = "protobuf_test_messages.proto3.TestAllTypesProto3";
  if (request.message_type == pb2) {
    using Message = protobuf_test_messages::proto2::TestAllTypesProto2;
    return run_test<Message>(request);
  }
  else if (request.message_type == pb3) {
    using Message = protobuf_test_messages::proto3::TestAllTypesProto3;
    return run_test<Message>(request);
  }
  else if (request.message_type == "conformance.FailureSet") {
    return run_test<conformance::FailureSet>(request);
  }
  else {
    return absl::NotFoundError(
        absl::StrCat("No such message type: ", request.message_type));
  }
}

absl::StatusOr<bool> Harness::ServeConformanceRequest() {
  uint32_t in_len;
  if (!ReadFd(STDIN_FILENO, reinterpret_cast<char*>(&in_len), sizeof(in_len))
           .ok()) {
    // EOF means we're done.
    return true;
  }

  std::string serialized_input;
  serialized_input.resize(in_len);
  RETURN_IF_ERROR(ReadFd(STDIN_FILENO, &serialized_input[0], in_len));

  ConformanceRequest request{};
  const auto& buffer = serialized_input;
  bool ok = struct_pb::internal::deserialize_to(request, buffer.data(),
                                                buffer.size());
  if (!ok) {
    return false;
  }

  absl::StatusOr<ConformanceResponse> response = RunTest(request);
  RETURN_IF_ERROR(response.status());

  std::string serialized_output = serialize(*response);

  uint32_t out_len = static_cast<uint32_t>(serialized_output.size());
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, &out_len, sizeof(out_len)));
  RETURN_IF_ERROR(WriteFd(STDOUT_FILENO, serialized_output.data(), out_len));
  if (verbose_) {
    std::cerr << "conformance-cpp: request=";
    std::cerr << request.message_type << std::endl;
  }
  return false;
}

int main() {
  Harness harness;
  int total_runs = 0;
  while (true) {
    auto is_done = harness.ServeConformanceRequest();
    if (!is_done.ok()) {
      std::cerr << is_done.status();
    }
    if (*is_done) {
      break;
    }
    total_runs++;
  }
  std::cerr << "conformance-struct_pb: received EOF from test runner after ";
  std::cerr << total_runs << " tests" << std::endl;
  return 0;
}

//
// the following code is useful for debugging when struct_pb conformance test
// failed.
//

std::string read_file(const std::string& tag, int index,
                      const std::string& suffix) {
  std::string filename = tag + "/" + std::to_string(index) + "." + suffix;
  std::ifstream ifs;
  ifs.open(filename, std::ios::in | std::ios::binary);
  std::size_t sz = fs::file_size(filename);
  std::string buffer;
  buffer.resize(sz);
  ifs.read(buffer.data(), buffer.size());
  return buffer;
}
int debug_struct_pb() {
  Harness harness;
  int index = 0;
  while (index < 1283) {
    // use data generated by conformance_cpp
    auto in_buf = read_file("in", index, "bin");
    auto out_buf = read_file("out", index, "bin");
    ConformanceRequest req{};
    bool ok =
        struct_pb::internal::deserialize_to(req, in_buf.data(), in_buf.size());
    if (!ok) {
      std::cout << "ERROR: " << index << " decode" << std::endl;
      return 1;
    }
    auto ret = harness.RunTest(req);
    if (!ret.ok()) {
      std::cout << "ERROR: " << index << " run test" << std::endl;
      return 2;
    }
    auto buf = serialize(*ret);
    if (buf != out_buf) {
      std::cout << "ERROR: " << index << " test fail" << std::endl;
      return 3;
    }
    index++;
  }
  return 0;
}
