// protoc generate parameter
// clang-format off
// 
// =========================
#include "test_large_enum_value.struct_pb.h"
#include "struct_pb/struct_pb/struct_pb_impl.hpp"
namespace struct_pb {
namespace internal {
// ::protobuf_unittest::TestLargeEnumValue
std::size_t get_needed_size(const ::protobuf_unittest::TestLargeEnumValue& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t total = unknown_fields.total_size();
  return total;
} // std::size_t get_needed_size(const ::protobuf_unittest::TestLargeEnumValue& t, const ::struct_pb::UnknownFields& unknown_fields)
void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestLargeEnumValue& t, const ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  unknown_fields.serialize_to(data, pos, size);
} // void serialize_to(char* data, std::size_t size, const ::protobuf_unittest::TestLargeEnumValue& t, const ::struct_pb::UnknownFields& unknown_fields)
bool deserialize_to(::protobuf_unittest::TestLargeEnumValue& t, const char* data, std::size_t size, ::struct_pb::UnknownFields& unknown_fields) {
  std::size_t pos = 0;
  bool ok = false;
  while (pos < size) {
    uint64_t tag = 0;
    ok = read_tag(data, pos, size, tag);
    if (!ok) {
      return false;
    }
    switch(tag) {
      default: {
        ok = deserialize_unknown(data, pos, size, tag, unknown_fields);
        return ok;
      }
    }
  }
return true;
} // bool deserialize_to(::protobuf_unittest::TestLargeEnumValue&, const char*, std::size_t)
// end of ::protobuf_unittest::TestLargeEnumValue
bool deserialize_to(::protobuf_unittest::TestLargeEnumValue& t, const char* data, std::size_t size) {
  ::struct_pb::UnknownFields unknown_fields{};
  return deserialize_to(t,data,size,unknown_fields);
}

} // internal
} // struct_pb
