#include "doctest.h"
#include "helper.hpp"
#include "struct_pb/struct_pb.hpp"
#ifdef HAVE_PROTOBUF
#include "test_pb.pb.h"
#endif
#include "test_pb.struct_pb.h"
using namespace doctest;
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::SubMessageForOneof, SubMessageForOneof>
    : public std::binary_function<test_struct_pb::SubMessageForOneof,
                                  SubMessageForOneof, bool> {
  constexpr bool operator()(const test_struct_pb::SubMessageForOneof& t,
                            const SubMessageForOneof& pb_t) const {
    return t.ok == pb_t.ok();
  }
};
template <>
struct PB_equal<test_struct_pb::SampleMessageOneof, SampleMessageOneof>
    : public std::binary_function<test_struct_pb::SampleMessageOneof,
                                  SampleMessageOneof, bool> {
  constexpr bool operator()(const test_struct_pb::SampleMessageOneof& t,
                            const SampleMessageOneof& pb_t) const {
    if (int(t.test_oneof_case()) != int(pb_t.test_oneof_case())) {
      return false;
    }
    switch (pb_t.test_oneof_case()) {
      case SampleMessageOneof::kB: {
        return t.b() == pb_t.b();
      } break;
      case SampleMessageOneof::kA: {
        return t.a() == pb_t.a();
      } break;
      case SampleMessageOneof::kName: {
        return t.name() == pb_t.name();
      } break;
      case SampleMessageOneof::kSubMessage: {
        return PB_equal<test_struct_pb::SubMessageForOneof,
                        SubMessageForOneof>()(*t.sub_message(),
                                              pb_t.sub_message());
      } break;
      case SampleMessageOneof::TEST_ONEOF_NOT_SET:
        break;
    }
    return true;
  }
};
#endif

namespace std {
template <>
struct equal_to<test_struct_pb::SampleMessageOneof>
    : public binary_function<test_struct_pb::SampleMessageOneof,
                             test_struct_pb::SampleMessageOneof, bool> {
  bool operator()(const test_struct_pb::SampleMessageOneof& lhs,
                  const test_struct_pb::SampleMessageOneof& rhs) const {
    if (lhs.test_oneof_case() != rhs.test_oneof_case()) {
      return false;
    }
    if (lhs.has_sub_message() && rhs.has_sub_message()) {
      return equal_to<test_struct_pb::SubMessageForOneof>()(*lhs.sub_message(),
                                                            *rhs.sub_message());
    }
    return lhs.test_oneof == rhs.test_oneof;
  }
};
}  // namespace std

TEST_CASE("testing oneof, b") {
  test_struct_pb::SampleMessageOneof t;
  int b = 13298;
  t.set_b(b);
  check_self(t);
#ifdef HAVE_PROTOBUF
  SampleMessageOneof pb_t;
  pb_t.set_b(b);
  check_with_protobuf(t, pb_t);
#endif
}
TEST_CASE("testing oneof, a") {
  test_struct_pb::SampleMessageOneof t;
  int a = 66613298;
  t.set_a(a);
  check_self(t);
#ifdef HAVE_PROTOBUF
  SampleMessageOneof pb_t;
  pb_t.set_a(a);
  check_with_protobuf(t, pb_t);
#endif
}
TEST_CASE("testing oneof, name") {
  std::string name = "oneof, name";
  test_struct_pb::SampleMessageOneof t;
  t.set_name(name);
  check_self(t);
#ifdef HAVE_PROTOBUF
  SampleMessageOneof pb_t;
  pb_t.set_name(name);
  check_with_protobuf(t, pb_t);
#endif
}

TEST_CASE("testing oneof, submessage") {
  test_struct_pb::SampleMessageOneof t;
  auto sub = new test_struct_pb::SubMessageForOneof{true};
  t.set_allocated_sub_message(sub);
  check_self(t);
#ifdef HAVE_PROTOBUF
  SampleMessageOneof pb_t;
  auto m = new SubMessageForOneof();
  m->set_ok(true);
  pb_t.set_allocated_sub_message(m);
  check_with_protobuf(t, pb_t);
#endif
}
