// protoc generate parameter
// generate_eq_op=true,namespace=test_struct_pb
// =========================
// clang-format off
#include "struct_pb/struct_pb.hpp"
// Generated by the protocol buffer compiler (struct_pb).  DO NOT EDIT!
// source: test_pb.proto

#pragma once

#include <limits>
#include <string>
#include <type_traits>
#include <memory>
#include <map>
#include <variant>
#include <vector>

namespace test_struct_pb {
struct Test1;
struct Test2;
struct Test3;
struct Test4;
struct MyTestDouble;
struct MyTestFloat;
struct MyTestInt32;
struct MyTestInt64;
struct MyTestUint32;
struct MyTestUint64;
struct MyTestEnum;
struct MyTestRepeatedMessage;
struct MyTestSint32;
struct MyTestSint64;
struct MyTestMap;
struct MyTestFixed32;
struct MyTestFixed64;
struct MyTestSfixed32;
struct MyTestSfixed64;
struct MyTestFieldNumberRandom;
struct MyTestAll;
struct SubMessageForOneof;
struct SampleMessageOneof;
struct Test1 {
  int32_t a; // int32, field number = 1
  bool operator==(const Test1&) const = default;
};
struct Test2 {
  std::string b; // string, field number = 2
  bool operator==(const Test2&) const = default;
};
struct Test3 {
  std::unique_ptr<::test_struct_pb::Test1> c; // message, field number = 3
  bool operator==(const Test3&) const = default;
};
struct Test4 {
  std::string d; // string, field number = 4
  std::vector<int32_t> e; // int32, field number = 5
  bool operator==(const Test4&) const = default;
};
struct MyTestDouble {
  double a; // double, field number = 1
  double b; // double, field number = 2
  double c; // double, field number = 3
  bool operator==(const MyTestDouble&) const = default;
};
struct MyTestFloat {
  float a; // float, field number = 1
  float b; // float, field number = 2
  float c; // float, field number = 3
  float d; // float, field number = 4
  float e; // float, field number = 5
  bool operator==(const MyTestFloat&) const = default;
};
struct MyTestInt32 {
  int32_t a; // int32, field number = 1
  std::vector<int32_t> b; // int32, field number = 2
  bool operator==(const MyTestInt32&) const = default;
};
struct MyTestInt64 {
  int64_t a; // int64, field number = 1
  std::vector<int64_t> b; // int64, field number = 2
  bool operator==(const MyTestInt64&) const = default;
};
struct MyTestUint32 {
  uint32_t a; // uint32, field number = 1
  std::vector<uint32_t> b; // uint32, field number = 2
  bool operator==(const MyTestUint32&) const = default;
};
struct MyTestUint64 {
  uint64_t a; // uint64, field number = 1
  std::vector<uint64_t> b; // uint64, field number = 2
  bool operator==(const MyTestUint64&) const = default;
};
struct MyTestEnum {
  enum class Color: int {
    Red = 0,
    Green = 1,
    Blue = 2,
    Enum127 = 127,
    Enum128 = 128,
  };
  ::test_struct_pb::MyTestEnum::Color color; // enum, field number = 6
  bool operator==(const MyTestEnum&) const = default;
};
struct MyTestRepeatedMessage {
  std::vector<::test_struct_pb::MyTestFloat> fs; // message, field number = 1
  bool operator==(const MyTestRepeatedMessage&) const = default;
};
struct MyTestSint32 {
  int32_t a; // sint32, field number = 1
  bool operator==(const MyTestSint32&) const = default;
};
struct MyTestSint64 {
  int64_t b; // sint64, field number = 2
  bool operator==(const MyTestSint64&) const = default;
};
struct MyTestMap {
  std::map<std::string, int32_t> e; // message, field number = 3
  bool operator==(const MyTestMap&) const = default;
};
struct MyTestFixed32 {
  uint32_t a; // fixed32, field number = 1
  std::vector<uint32_t> b; // fixed32, field number = 2
  bool operator==(const MyTestFixed32&) const = default;
};
struct MyTestFixed64 {
  uint64_t a; // fixed64, field number = 1
  std::vector<uint64_t> b; // fixed64, field number = 2
  bool operator==(const MyTestFixed64&) const = default;
};
struct MyTestSfixed32 {
  int32_t a; // sfixed32, field number = 1
  std::vector<int32_t> b; // sfixed32, field number = 2
  bool operator==(const MyTestSfixed32&) const = default;
};
struct MyTestSfixed64 {
  int64_t a; // sfixed64, field number = 1
  std::vector<int64_t> b; // sfixed64, field number = 2
  bool operator==(const MyTestSfixed64&) const = default;
};
struct MyTestFieldNumberRandom {
  int32_t a; // int32, field number = 6
  int64_t b; // sint64, field number = 3
  std::string c; // string, field number = 4
  double d; // double, field number = 5
  float e; // float, field number = 1
  std::vector<uint32_t> f; // fixed32, field number = 128
  bool operator==(const MyTestFieldNumberRandom&) const = default;
};
struct MyTestAll {
  double a; // double, field number = 1
  float b; // float, field number = 2
  int32_t c; // int32, field number = 3
  int64_t d; // int64, field number = 4
  uint32_t e; // uint32, field number = 5
  uint64_t f; // uint64, field number = 6
  int32_t g; // sint32, field number = 7
  int64_t h; // sint64, field number = 8
  uint32_t i; // fixed32, field number = 9
  uint64_t j; // fixed64, field number = 10
  int32_t k; // sfixed32, field number = 11
  int64_t l; // sfixed64, field number = 12
  bool m; // bool, field number = 13
  std::string n; // string, field number = 14
  std::string o; // bytes, field number = 15
  bool operator==(const MyTestAll&) const = default;
};
struct SubMessageForOneof {
  bool ok; // bool, field number = 1
  bool operator==(const SubMessageForOneof&) const = default;
};
struct SampleMessageOneof {
  enum class TestOneofCase {
    none = 0,
    b = 10,
    a = 8,
    name = 4,
    sub_message = 9,
  };
  TestOneofCase test_oneof_case() const {
    switch (test_oneof.index()) {
      case 1:
        return TestOneofCase::b;
      case 2:
        return TestOneofCase::a;
      case 3:
        return TestOneofCase::name;
      case 4:
        return TestOneofCase::sub_message;
      default:
        return TestOneofCase::none;
    }
  }

  std::variant<std::monostate
  , int32_t // int32, field number = 10
  , int32_t // int32, field number = 8
  , std::string // string, field number = 4
  , std::unique_ptr<::test_struct_pb::SubMessageForOneof> // message, field number = 9
  > test_oneof;
  bool has_b() const {
    return test_oneof.index() == 1;
  }
  void set_b(int32_t b) {
    test_oneof.emplace<1>(b);
  }
  int32_t b() const {
    assert(test_oneof.index() == 1);
    return std::get<1>(test_oneof);
  }
  bool has_a() const {
    return test_oneof.index() == 2;
  }
  void set_a(int32_t a) {
    test_oneof.emplace<2>(a);
  }
  int32_t a() const {
    assert(test_oneof.index() == 2);
    return std::get<2>(test_oneof);
  }
  bool has_name() const {
    return test_oneof.index() == 3;
  }
  void set_name(std::string name) {
    test_oneof.emplace<3>(std::move(name));
  }
  const std::string& name() const {
    assert(test_oneof.index() == 3);
    return std::get<3>(test_oneof);
  }
  bool has_sub_message() const {
    return test_oneof.index() == 4;
  }
  void set_allocated_sub_message(::test_struct_pb::SubMessageForOneof* p) {
    assert(p);
    test_oneof.emplace<4>(p);
  }
  const std::unique_ptr<::test_struct_pb::SubMessageForOneof>& sub_message() const {
    assert(test_oneof.index() == 4);
    return std::get<4>(test_oneof);
  }
  bool operator==(const SampleMessageOneof&) const = default;
};
} // namespace test_struct_pb
namespace struct_pb {
namespace internal {
// ::test_struct_pb::Test1
std::size_t get_needed_size(const ::test_struct_pb::Test1&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::Test1&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::Test1&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::Test1&, const char*, std::size_t);

// ::test_struct_pb::Test2
std::size_t get_needed_size(const ::test_struct_pb::Test2&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::Test2&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::Test2&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::Test2&, const char*, std::size_t);

// ::test_struct_pb::Test3
std::size_t get_needed_size(const ::test_struct_pb::Test3&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::Test3&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::Test3&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::Test3&, const char*, std::size_t);

// ::test_struct_pb::Test4
std::size_t get_needed_size(const ::test_struct_pb::Test4&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::Test4&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::Test4&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::Test4&, const char*, std::size_t);

// ::test_struct_pb::MyTestDouble
std::size_t get_needed_size(const ::test_struct_pb::MyTestDouble&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestDouble&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestDouble&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestDouble&, const char*, std::size_t);

// ::test_struct_pb::MyTestFloat
std::size_t get_needed_size(const ::test_struct_pb::MyTestFloat&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestFloat&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestFloat&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestFloat&, const char*, std::size_t);

// ::test_struct_pb::MyTestInt32
std::size_t get_needed_size(const ::test_struct_pb::MyTestInt32&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestInt32&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestInt32&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestInt32&, const char*, std::size_t);

// ::test_struct_pb::MyTestInt64
std::size_t get_needed_size(const ::test_struct_pb::MyTestInt64&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestInt64&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestInt64&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestInt64&, const char*, std::size_t);

// ::test_struct_pb::MyTestUint32
std::size_t get_needed_size(const ::test_struct_pb::MyTestUint32&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestUint32&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestUint32&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestUint32&, const char*, std::size_t);

// ::test_struct_pb::MyTestUint64
std::size_t get_needed_size(const ::test_struct_pb::MyTestUint64&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestUint64&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestUint64&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestUint64&, const char*, std::size_t);

// ::test_struct_pb::MyTestEnum
std::size_t get_needed_size(const ::test_struct_pb::MyTestEnum&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestEnum&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestEnum&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestEnum&, const char*, std::size_t);

// ::test_struct_pb::MyTestRepeatedMessage
std::size_t get_needed_size(const ::test_struct_pb::MyTestRepeatedMessage&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestRepeatedMessage&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestRepeatedMessage&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestRepeatedMessage&, const char*, std::size_t);

// ::test_struct_pb::MyTestSint32
std::size_t get_needed_size(const ::test_struct_pb::MyTestSint32&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestSint32&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestSint32&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestSint32&, const char*, std::size_t);

// ::test_struct_pb::MyTestSint64
std::size_t get_needed_size(const ::test_struct_pb::MyTestSint64&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestSint64&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestSint64&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestSint64&, const char*, std::size_t);

// ::test_struct_pb::MyTestMap
std::size_t get_needed_size(const ::test_struct_pb::MyTestMap&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestMap&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestMap&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestMap&, const char*, std::size_t);

// ::test_struct_pb::MyTestFixed32
std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed32&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestFixed32&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestFixed32&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestFixed32&, const char*, std::size_t);

// ::test_struct_pb::MyTestFixed64
std::size_t get_needed_size(const ::test_struct_pb::MyTestFixed64&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestFixed64&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestFixed64&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestFixed64&, const char*, std::size_t);

// ::test_struct_pb::MyTestSfixed32
std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed32&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestSfixed32&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestSfixed32&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestSfixed32&, const char*, std::size_t);

// ::test_struct_pb::MyTestSfixed64
std::size_t get_needed_size(const ::test_struct_pb::MyTestSfixed64&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestSfixed64&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestSfixed64&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestSfixed64&, const char*, std::size_t);

// ::test_struct_pb::MyTestFieldNumberRandom
std::size_t get_needed_size(const ::test_struct_pb::MyTestFieldNumberRandom&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestFieldNumberRandom&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestFieldNumberRandom&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestFieldNumberRandom&, const char*, std::size_t);

// ::test_struct_pb::MyTestAll
std::size_t get_needed_size(const ::test_struct_pb::MyTestAll&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::MyTestAll&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::MyTestAll&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::MyTestAll&, const char*, std::size_t);

// ::test_struct_pb::SubMessageForOneof
std::size_t get_needed_size(const ::test_struct_pb::SubMessageForOneof&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::SubMessageForOneof&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::SubMessageForOneof&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::SubMessageForOneof&, const char*, std::size_t);

// ::test_struct_pb::SampleMessageOneof
std::size_t get_needed_size(const ::test_struct_pb::SampleMessageOneof&, const ::struct_pb::UnknownFields& unknown_fields = {});
void serialize_to(char*, std::size_t, const ::test_struct_pb::SampleMessageOneof&, const ::struct_pb::UnknownFields& unknown_fields = {});
bool deserialize_to(::test_struct_pb::SampleMessageOneof&, const char*, std::size_t, ::struct_pb::UnknownFields& unknown_fields);
bool deserialize_to(::test_struct_pb::SampleMessageOneof&, const char*, std::size_t);

} // internal
} // struct_pb
// clang-format on
