/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <valarray>

#include "doctest.h"
#include "helper.hpp"
#ifdef HAVE_PROTOBUF
#include "test_pb.pb.h"
#endif
using namespace doctest;
using namespace struct_pb;

TEST_SUITE_BEGIN("test pb");
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::Test1, Test1> {
  bool operator()(const test_struct_pb::Test1& t, const Test1& pb_t) const {
    return t.a == pb_t.a();
  }
};
#endif
TEST_CASE("testing test_struct_pb::Test1") {
  test_struct_pb::Test1 t{};
  SUBCASE("empty") { check_self(t); }
  SUBCASE("has value") {
    std::string buf{0x08, (char)0x96, 0x01};
    t.a = 150;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    Test1 pb_t;
    pb_t.set_a(150);
    auto pb_buf = pb_t.SerializeAsString();
    REQUIRE(pb_buf == buf);
    check_with_protobuf(t, pb_t);
#endif
  }

  SUBCASE("negative") {
    {
      // an example for check arm char
      std::string buf{0x08,       (char)0xff, (char)0xff, (char)0xff,
                      (char)0xff, (char)0xff, (char)0xff, (char)0xff,
                      (char)0xff, (char)0xff, 0x01};
      test_struct_pb::Test1 t{.a = -1};
      check_self(t, buf);
#ifdef HAVE_PROTOBUF
      // for debug
      Test1 pb_t;
      pb_t.set_a(-1);
      auto s = pb_t.SerializeAsString();
      check_with_protobuf(t, pb_t);
#endif
    }
    for (int32_t i = -1; i > INT16_MIN + 1; i *= 2) {
      test_struct_pb::Test1 t{.a = i};
      check_self(t);
#ifdef HAVE_PROTOBUF
      Test1 pb_t;
      pb_t.set_a(i);
      check_with_protobuf(t, pb_t);
#endif
    }
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::Test2, Test2> {
  bool operator()(const test_struct_pb::Test2& t, const Test2& pb_t) const {
    return t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing test_struct_pb::Test2") {
  test_struct_pb::Test2 t{};
  SUBCASE("empty") { check_self(t); }
  SUBCASE("has value") {
    std::string buf{0x12, 0x07};
    std::string s = "testing";
    buf += s;
    t.b = s;
    check_self(t, buf);

#ifdef HAVE_PROTOBUF
    Test2 pb_t;
    pb_t.set_b(s);
    check_with_protobuf(t, pb_t);
#endif
  }
}
namespace std {
template <>
struct equal_to<test_struct_pb::Test3> {
  bool operator()(const test_struct_pb::Test3& lhs,
                  const test_struct_pb::Test3& rhs) const {
    if (lhs.c && rhs.c) {
      return equal_to<test_struct_pb::Test1>()(*lhs.c, *rhs.c);
    }
    if (!lhs.c && !rhs.c) {
      return true;
    }
    return false;
  }
};
}  // namespace std
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::Test3, Test3> {
  bool operator()(const test_struct_pb::Test3& t, const Test3& pb_t) const {
    if (t.c && pb_t.has_c()) {
      return PB_equal<test_struct_pb::Test1, Test1>()(*t.c, pb_t.c());
    }
    if (!t.c && !pb_t.has_c()) {
      return true;
    }
    return false;
  }
};
#endif
TEST_CASE("testing test_struct_pb::Test3") {
  test_struct_pb::Test3 t{};
  SUBCASE("empty") { check_self(t); }
  SUBCASE("has value") {
    std::string buf{0x1a, 0x03, 0x08, (char)0x96, 0x01};
    t.c = std::make_unique<test_struct_pb::Test1>(
        test_struct_pb::Test1{.a = 150});
    check_self(t, buf);

#ifdef HAVE_PROTOBUF
    Test3 pb_t;
    auto pb_c = new Test1;
    pb_c->set_a(150);
    pb_t.set_allocated_c(pb_c);

    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::Test4, Test4> {
  bool operator()(const test_struct_pb::Test4& t, const Test4& pb_t) const {
    return t.e == pb_t.e();
  }
};
#endif
TEST_CASE("testing test_struct_pb::Test4") {
  test_struct_pb::Test4 t{};
  SUBCASE("empty") { check_self(t); }
  SUBCASE("string empty") {
    std::string buf{0x2a, 0x01, 0x01};
    t.e = {1};
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    Test4 pb_t;
    pb_t.add_e(1);
    auto pb_buf = pb_t.SerializeAsString();
    REQUIRE(pb_buf == buf);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("repeated empty") {
    // [4] 22 2 68 69
    std::string buf{0x22, 0x02, 0x68, 0x69};
    std::string s = "hi";
    t.d = s;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    Test4 pb_t;
    pb_t.set_d(s);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("has value") {
    std::string buf{0x22, 0x05, 0x68, 0x65, 0x6c, 0x6c,
                    0x6f, 0x2a, 0x03, 0x01, 0x02, 0x03};
    // why document write
    // 220568656c6c6f280128022803
    // while my pb c++ code generated
    // 34 5 'h' 'e' 'l' 'l' 'o' 42 3 1 2 3
    // 22 5 68 65 6c 6c 6f 2a 3 1 2 3
    t.d = "hello";
    t.e = {1, 2, 3};
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    Test4 pb_t;
    pb_t.set_d("hello");
    pb_t.add_e(1);
    pb_t.add_e(2);
    pb_t.add_e(3);
    check_with_protobuf(t, pb_t);
#endif
  }
}

#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestDouble, MyTestDouble> {
  bool operator()(const test_struct_pb::MyTestDouble& t,
                  const MyTestDouble& pb_t) const {
    std::valarray<double> ts{t.a, t.b, t.c};
    std::valarray<double> pb_ts{pb_t.a(), pb_t.b(), pb_t.c()};
    return (std::abs(ts - pb_ts) < 0.00005f).min();
  }
};
#endif
TEST_CASE("testing double") {
  test_struct_pb::MyTestDouble t{.a = 123.456, .b = 0, .c = -678.123};
  check_self(t);
#ifdef HAVE_PROTOBUF
  MyTestDouble pb_t;
  pb_t.set_a(t.a);
  pb_t.set_b(t.b);
  pb_t.set_c(t.c);
  check_with_protobuf(t, pb_t);
#endif
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestFloat, MyTestFloat> {
  bool operator()(const test_struct_pb::MyTestFloat& t,
                  const MyTestFloat& pb_t) const {
    std::valarray<float> ts{t.a, t.b, t.c, t.d, t.e};
    std::valarray<float> pb_ts{pb_t.a(), pb_t.b(), pb_t.c(), pb_t.d(),
                               pb_t.e()};
    return (std::abs(ts - pb_ts) < 0.00005f).min();
  }
};
#endif
TEST_CASE("testing float") {
  // [20] 15 00 00 80 3f 1d 00 00 80 bf 25 b6 f3 9d 3f 2d 00 04 f1 47
  std::string buf{0x15, 0x00,       0x00,       (char)0x80, 0x3f,
                  0x1d, 0x00,       0x00,       (char)0x80, (char)0xbf,
                  0x25, (char)0xb6, (char)0xf3, (char)0x9d, 0x3f,
                  0x2d, 0x00,       0x04,       (char)0xf1, 0x47};
  test_struct_pb::MyTestFloat t{
      .a = 0, .b = 1, .c = -1, .d = 1.234, .e = 1.234e5};
  check_self(t, buf);
#ifdef HAVE_PROTOBUF
  MyTestFloat pb_t;
  pb_t.set_a(0);
  pb_t.set_b(1);
  pb_t.set_c(-1);
  pb_t.set_d(1.234);
  pb_t.set_e(1.234e5);
  check_with_protobuf(t, pb_t);
#endif
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestInt32, MyTestInt32> {
  bool operator()(const test_struct_pb::MyTestInt32& t,
                  const MyTestInt32& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing int32") {
  std::string buf{0x08, (char)0x80, 0x01};
  test_struct_pb::MyTestInt32 t;
  t.a = 128;
  check_self(t, buf);
#ifdef HAVE_PROTOBUF
  MyTestInt32 pb_t;
  pb_t.set_a(128);
  check_with_protobuf(t, pb_t);
#endif
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestInt64, MyTestInt64> {
  bool operator()(const test_struct_pb::MyTestInt64& t,
                  const MyTestInt64& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing int64") {
  int64_t max_val = 4;
  max_val *= INT32_MAX;
  for (int64_t i = 1; i < max_val; i *= -2) {
    test_struct_pb::MyTestInt64 t{.a = i};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestInt64 pb_t;
    pb_t.set_a(i);
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestUint32, MyTestUint32> {
  bool operator()(const test_struct_pb::MyTestUint32& t,
                  const MyTestUint32& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing uint32") {
  int32_t max_val = 4;
  max_val *= INT16_MAX;
  for (uint32_t i = 1; i < max_val; i *= 2) {
    test_struct_pb::MyTestUint32 t{.a = i};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestUint32 pb_t;
    pb_t.set_a(i);
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestUint64, MyTestUint64> {
  bool operator()(const test_struct_pb::MyTestUint64& t,
                  const MyTestUint64& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing uint64") {
  int64_t max_val = 4;
  max_val *= INT32_MAX;
  for (uint64_t i = 1; i < max_val; i *= 2) {
    test_struct_pb::MyTestUint64 t{.a = i};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestUint64 pb_t;
    pb_t.set_a(i);
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestEnum, MyTestEnum> {
  bool operator()(const test_struct_pb::MyTestEnum& t,
                  const MyTestEnum& pb_t) const {
    return static_cast<uint32_t>(t.color) ==
           static_cast<uint32_t>(pb_t.color());
  }
};
#endif
TEST_CASE("testing enum") {
  SUBCASE("Red") {
    std::string buf;
    test_struct_pb::MyTestEnum t{};
    t.color = test_struct_pb::MyTestEnum::Color::Red;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestEnum pb_t;
    pb_t.set_color(MyTestEnum_Color::MyTestEnum_Color_Red);
    auto pb_buf = pb_t.SerializeAsString();
    REQUIRE(hex_helper(pb_buf) == hex_helper(buf));
    MyTestEnum pb_d_t;
    auto ok = pb_d_t.ParseFromArray(pb_buf.data(), pb_buf.size());
    REQUIRE(ok);
    CHECK(pb_d_t.IsInitialized());
    CHECK(pb_d_t.color() == pb_t.color());
    auto c = pb_d_t.color();
    CHECK(c == MyTestEnum_Color::MyTestEnum_Color_Red);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("Green") {
    std::string buf{0x30, 0x01};
    test_struct_pb::MyTestEnum t{};
    t.color = test_struct_pb::MyTestEnum::Color::Green;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestEnum pb_t;
    pb_t.set_color(MyTestEnum_Color::MyTestEnum_Color_Green);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("Blue") {
    std::string buf{0x30, 0x02};
    test_struct_pb::MyTestEnum t{};
    t.color = test_struct_pb::MyTestEnum::Color::Blue;
    check_self(t, buf);

#ifdef HAVE_PROTOBUF
    MyTestEnum pb_t;
    pb_t.set_color(MyTestEnum_Color::MyTestEnum_Color_Blue);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("Enum127") {
    std::string buf{0x30, (char)0x7f};
    test_struct_pb::MyTestEnum t{};
    t.color = test_struct_pb::MyTestEnum::Color::Enum127;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestEnum pb_t;
    pb_t.set_color(MyTestEnum_Color::MyTestEnum_Color_Enum127);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("Enum128") {
    std::string buf{0x30, (char)0x80, 0x01};
    test_struct_pb::MyTestEnum t{};
    t.color = test_struct_pb::MyTestEnum::Color::Enum128;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestEnum pb_t;
    pb_t.set_color(MyTestEnum_Color::MyTestEnum_Color_Enum128);
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestRepeatedMessage, MyTestRepeatedMessage> {
  bool operator()(const test_struct_pb::MyTestRepeatedMessage& t,
                  const MyTestRepeatedMessage& pb_t) const {
    return t.fs == pb_t.fs();
  }
};
#endif
TEST_CASE("testing nested repeated message") {
  SUBCASE("one") {
    test_struct_pb::MyTestRepeatedMessage t{};
    t.fs = {{1, 2, 3}};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestRepeatedMessage pb_t;
    {
      auto f1 = pb_t.add_fs();
      f1->set_a(1);
      f1->set_b(2);
      f1->set_c(3);
    }
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("two") {
    test_struct_pb::MyTestRepeatedMessage t{};
    t.fs = {{1, 2, 3}, {4, 5, 6}};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestRepeatedMessage pb_t;
    {
      auto f1 = pb_t.add_fs();
      f1->set_a(1);
      f1->set_b(2);
      f1->set_c(3);
    }
    {
      auto f1 = pb_t.add_fs();
      f1->set_a(4);
      f1->set_b(5);
      f1->set_c(6);
    }
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestSint32, MyTestSint32> {
  bool operator()(const test_struct_pb::MyTestSint32& t,
                  const MyTestSint32& pb_t) const {
    return t.a == pb_t.a();
  }
};
#endif
TEST_CASE("testing sint32") {
  SUBCASE("-1") {
    std::string buf{0x08, 0x01};
    test_struct_pb::MyTestSint32 t{};
    t.a = -1;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestSint32 pb_t;
    pb_t.set_a(-1);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("1") {
    test_struct_pb::MyTestSint32 t{};
    t.a = 1;
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestSint32 pb_t;
    pb_t.set_a(1);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("128") {
    std::string buf{0x08, (char)0x80, 0x02};
    test_struct_pb::MyTestSint32 t{};
    t.a = 128;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestSint32 pb_t;
    pb_t.set_a(128);
    check_with_protobuf(t, pb_t);
#endif
  }

  SUBCASE("range") {
    for (int32_t i = INT16_MAX; i > INT16_MIN + 1; --i) {
      test_struct_pb::MyTestSint32 t{.a = i};
      check_self(t);
#ifdef HAVE_PROTOBUF
      MyTestSint32 pb_t;
      pb_t.set_a(i);
      check_with_protobuf(t, pb_t);
#endif
    }
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestSint64, MyTestSint64> {
  bool operator()(const test_struct_pb::MyTestSint64& t,
                  const MyTestSint64& pb_t) const {
    return t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing sint64") {
  SUBCASE("-1") {
    std::string buf{0x10, 0x01};
    test_struct_pb::MyTestSint64 t{};
    t.b = -1;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestSint64 pb_t;
    pb_t.set_b(-1);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("128") {
    std::string buf{0x10, (char)0x80, 0x02};
    test_struct_pb::MyTestSint64 t{};
    t.b = 128;
    check_self(t, buf);

#ifdef HAVE_PROTOBUF
    MyTestSint64 pb_t;
    pb_t.set_b(128);
    check_with_protobuf(t, pb_t);

#endif
  }

  SUBCASE("range") {
    for (int64_t i = -1; i > INT32_MIN + 1; i *= -2) {
      test_struct_pb::MyTestSint64 t{.b = i};
      check_self(t);
#ifdef HAVE_PROTOBUF
      MyTestSint64 pb_t;
      pb_t.set_b(i);
      check_with_protobuf(t, pb_t);
#endif
    }
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestMap, MyTestMap> {
  bool operator()(const test_struct_pb::MyTestMap& t,
                  const MyTestMap& pb_t) const {
    if (t.e.size() != pb_t.e_size()) {
      return false;
    }
    for (const auto& [k, v] : pb_t.e()) {
      auto it = t.e.find(k);
      if (it == t.e.end()) {
        return false;
      }
      if (it->second != v) {
        return false;
      }
    }
    return true;
  }
};
#endif
TEST_CASE("testing map") {
  SUBCASE("one entry") {
    std::string buf{0x1a, 0x05, 0x0a, 0x01, 0x61, 0x10, 0x01};
    test_struct_pb::MyTestMap t;
    t.e["a"] = 1;
    check_self(t, buf);
#ifdef HAVE_PROTOBUF
    MyTestMap pb_t;
    auto& pb_m = *pb_t.mutable_e();
    pb_m["a"] = 1;
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("two entry") {
    std::string buf{
        0x1a,       0x0a, 0x0a, 0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x10,
        (char)0x9a, 0x05, 0x1a, 0x05, 0x0a, 0x01, 0x61, 0x10, 0x01,
    };
    std::string buf2{
        0x1a, 0x05, 0x0a, 0x01, 0x61, 0x10, 0x01, 0x1a,       0x0a, 0x0a,
        0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x10, (char)0x9a, 0x05,
    };
    test_struct_pb::MyTestMap t;
    t.e["a"] = 1;
    t.e["hello"] = 666;
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestMap pb_t;
    auto& pb_m = *pb_t.mutable_e();
    pb_m["a"] = 1;
    pb_m["hello"] = 666;
    auto pb_buf = pb_t.SerializeAsString();

    CHECK(buf.size() == pb_buf.size());
    REQUIRE((buf == pb_buf || buf2 == pb_buf));
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestFixed32, MyTestFixed32> {
  bool operator()(const test_struct_pb::MyTestFixed32& t,
                  const MyTestFixed32& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing fixed32") {
  test_struct_pb::MyTestFixed32 t{};
  SUBCASE("single fixed") {
    t.a = 888;
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestFixed32 pb_t;
    pb_t.set_a(888);
    check_with_protobuf(t, pb_t);
#endif
  }
  SUBCASE("only repeated") {
    t.b = {5, 4, 3, 2, 1};
    check_self(t);
#ifdef HAVE_PROTOBUF
    MyTestFixed32 pb_t;
    pb_t.add_b(5);
    pb_t.add_b(4);
    pb_t.add_b(3);
    pb_t.add_b(2);
    pb_t.add_b(1);
    check_with_protobuf(t, pb_t);
#endif
  }
}

#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestFixed64, MyTestFixed64> {
  bool operator()(const test_struct_pb::MyTestFixed64& t,
                  const MyTestFixed64& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing fixed64") {
  using T = uint64_t;
  T max_val = 4;
  max_val *= INT32_MAX;
  for (T i = 1; i < max_val; i *= 2) {
    test_struct_pb::MyTestFixed64 t{.a = i};
#ifdef HAVE_PROTOBUF
    MyTestFixed64 pb_t;
    pb_t.set_a(i);
#endif
    for (T j = 1; j <= i; j *= 2) {
#ifdef HAVE_PROTOBUF
      pb_t.add_b(j * 3);
#endif
      t.b.push_back(j * 3);
    }
    check_self(t);
#ifdef HAVE_PROTOBUF
    check_with_protobuf(t, pb_t);
#endif
  }
}

#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestSfixed32, MyTestSfixed32> {
  bool operator()(const test_struct_pb::MyTestSfixed32& t,
                  const MyTestSfixed32& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing sfixed32") {
  using T = int32_t;
  T max_val = 4;
  max_val *= INT16_MAX;
  for (T i = 1; i < max_val; i *= -2) {
    test_struct_pb::MyTestSfixed32 t{.a = i};
#ifdef HAVE_PROTOBUF
    MyTestSfixed32 pb_t;
    pb_t.set_a(i);
#endif
    for (T j = 1; j <= i; j *= -2) {
#ifdef HAVE_PROTOBUF
      pb_t.add_b(j * -3);
#endif
      t.b.push_back(j * -3);
    }
    check_self(t);
#ifdef HAVE_PROTOBUF
    check_with_protobuf(t, pb_t);
#endif
  }
}

#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestSfixed64, MyTestSfixed64> {
  bool operator()(const test_struct_pb::MyTestSfixed64& t,
                  const MyTestSfixed64& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b();
  }
};
#endif
TEST_CASE("testing sfixed64") {
  using T = int64_t;
  T max_val = 4;
  max_val *= INT32_MAX;
  for (T i = 1; i < max_val; i *= -2) {
    test_struct_pb::MyTestSfixed64 t{.a = i};
#ifdef HAVE_PROTOBUF
    MyTestSfixed64 pb_t;
    pb_t.set_a(i);
#endif
    for (T j = 1; j <= i; j *= -2) {
#ifdef HAVE_PROTOBUF
      pb_t.add_b(j * -3);
#endif
      t.b.push_back(j * -3);
    }
    check_self(t);
#ifdef HAVE_PROTOBUF
    check_with_protobuf(t, pb_t);
#endif
  }
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestFieldNumberRandom,
                MyTestFieldNumberRandom> {
  bool operator()(const test_struct_pb::MyTestFieldNumberRandom& t,
                  const MyTestFieldNumberRandom& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b() && t.c == pb_t.c() &&
           t.f == pb_t.f() && std::abs(t.d - pb_t.d()) < 0.00005f &&
           std::abs(t.e - pb_t.e()) < 0.00005f;
  }
};
#endif
TEST_CASE("testing random field number") {
  test_struct_pb::MyTestFieldNumberRandom t{.a = 666,
                                            .b = 999,
                                            .c = "random",
                                            .d = 3.14,
                                            .e = 3344.123,
                                            .f = {5, 4, 3, 2, 1}};
  check_self(t);
#ifdef HAVE_PROTOBUF
  MyTestFieldNumberRandom pb_t;
  pb_t.set_a(666);
  pb_t.set_b(999);
  pb_t.set_c("random");
  pb_t.set_d(3.14);
  pb_t.set_e(3344.123);
  pb_t.add_f(5);
  pb_t.add_f(4);
  pb_t.add_f(3);
  pb_t.add_f(2);
  pb_t.add_f(1);
  check_with_protobuf(t, pb_t);
#endif
}
#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<test_struct_pb::MyTestAll, MyTestAll> {
  bool operator()(const test_struct_pb::MyTestAll& t,
                  const MyTestAll& pb_t) const {
    return t.a == pb_t.a() && t.b == pb_t.b() && t.c == pb_t.c() &&
           t.d == pb_t.d() && t.e == pb_t.e() && t.f == pb_t.f() &&
           t.g == pb_t.g() && t.h == pb_t.h() && t.i == pb_t.i() &&
           t.j == pb_t.j() && t.k == pb_t.k() && t.l == pb_t.l() &&
           t.m == pb_t.m() && t.n == pb_t.n() && t.o == pb_t.o();
  }
};
#endif
TEST_CASE("testing all") {
  test_struct_pb::MyTestAll t{};

  t.a = -100;
  t.b = -200;
  t.c = -300;
  t.d = -2378;
  t.e = 2323;
  t.f = 255;
  t.g = -890;
  t.h = -2369723;
  t.i = -23234;
  t.j = -239223423;
  t.k = 9232983;
  t.l = 237982374432;
  t.m = true;
  t.n = "testing all types";
  t.o = "\1\2\3\4\5\t\n666";
  check_self(t);
#ifdef HAVE_PROTOBUF
  MyTestAll pb_t;
  pb_t.set_a(t.a);
  pb_t.set_b(t.b);
  pb_t.set_c(t.c);
  pb_t.set_d(t.d);
  pb_t.set_e(t.e);
  pb_t.set_f(t.f);
  pb_t.set_g(t.g);
  pb_t.set_h(t.h);
  pb_t.set_i(t.i);
  pb_t.set_j(t.j);
  pb_t.set_k(t.k);
  pb_t.set_l(t.l);
  pb_t.set_m(t.m);
  pb_t.set_n(t.n);
  pb_t.set_o(t.o);

  check_with_protobuf(t, pb_t);
#endif
}
TEST_SUITE_END;