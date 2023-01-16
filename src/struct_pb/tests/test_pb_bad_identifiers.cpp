#include "doctest.h"
#include "test_bad_identifiers.struct_pb.h"
using namespace doctest;

TEST_SUITE_BEGIN("test bad identifiers");
TEST_CASE("testing conflicting symbol names") {
  protobuf_unittest::TestConflictingSymbolNames message;
  message.uint32 = 1;
  auto sz = struct_pb::internal::get_needed_size(message);
  CHECK(sz == 3);
  // Due to different API, the code is useless
  message.friend_ = 5;
  CHECK(message.friend_ == 5);
  message.class_ = 6;
  CHECK(message.class_ == 6);
  // NO PROTO3
  typedef protobuf_unittest::TestConflictingSymbolNamesExtension
      ExtensionMessage;
}
TEST_CASE("testing conflicting enum names") {
  protobuf_unittest::TestConflictingEnumNames message;
  message.conflicting_enum =
      protobuf_unittest::TestConflictingEnumNames::while_::and_;
  CHECK(1 == static_cast<int>(message.conflicting_enum.value()));

  message.conflicting_enum =
      protobuf_unittest::TestConflictingEnumNames::while_::XOR;
  CHECK(5 == static_cast<int>(message.conflicting_enum.value()));

  protobuf_unittest::bool_ conflicting_enum;
  conflicting_enum = protobuf_unittest::bool_::NOT_EQ;
  CHECK(1 == static_cast<int>(conflicting_enum));

  conflicting_enum = protobuf_unittest::bool_::return_;
  CHECK(3 == static_cast<int>(conflicting_enum));
}
TEST_CASE("testing conflicting message names") {
  protobuf_unittest::NULL_ message;
  message.int_ = 123;
  CHECK(message.int_ == 123);
}
TEST_CASE("testing conflicting extension") {
  protobuf_unittest::TestConflictingSymbolNames message;
  // NO_PROTO3
}
TEST_SUITE_END();