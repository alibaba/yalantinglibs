#include "struct_pack/struct_pack/pb.hpp"
#include "doctest.h"
#include "test_pb.pb.h"
#include "hex_printer.hpp"

using namespace doctest;

TEST_CASE("testing oneof") {
  SUBCASE("index 0") {
    SampleMessageOneof pb_t;
    pb_t.set_b(13298);
    auto pb_buf = pb_t.SerializeAsString();

    sample_message_oneof t;
    t.test_oneof.emplace<0>(13298);
    auto size = get_needed_size(t);
    REQUIRE(size == pb_buf.size());
    auto buf = serialize<std::string>(t);
    REQUIRE(buf == pb_buf);
    std::size_t len = 0;
    auto d_t_ret = deserialize<sample_message_oneof>(buf.data(), buf.size(), len);
    REQUIRE(d_t_ret);
    REQUIRE(len == buf.size());
    auto d_t = d_t_ret.value();
    CHECK(d_t.test_oneof.index() == 0);
    CHECK(std::get<0>(d_t.test_oneof) == 13298);
  }
  SUBCASE("index 1") {

    SampleMessageOneof pb_t;
    pb_t.set_a(66613298);
    auto pb_buf = pb_t.SerializeAsString();

    sample_message_oneof t;
    t.test_oneof.emplace<1>(66613298);
    auto size = get_needed_size(t);
    REQUIRE(size == pb_buf.size());
    auto buf = serialize<std::string>(t);
    REQUIRE(buf == pb_buf);
    std::size_t len = 0;
    auto d_t_ret = deserialize<sample_message_oneof>(buf.data(), buf.size(), len);
    REQUIRE(d_t_ret);
    REQUIRE(len == buf.size());
    auto d_t = d_t_ret.value();
    CHECK(d_t.test_oneof.index() == 1);
    CHECK(std::get<1>(d_t.test_oneof) == 66613298);
  }
  SUBCASE("index 2") {
    SampleMessageOneof pb_t;
    pb_t.set_name("oneof index 2");
    auto pb_buf = pb_t.SerializeAsString();

    sample_message_oneof t;
    t.test_oneof.emplace<2>("oneof index 2");
    auto size = get_needed_size(t);
    REQUIRE(size == pb_buf.size());
    auto buf = serialize<std::string>(t);
    REQUIRE(buf == pb_buf);
    std::size_t len = 0;
    auto d_t_ret = deserialize<sample_message_oneof>(buf.data(), buf.size(), len);
    REQUIRE(d_t_ret);
    REQUIRE(len == buf.size());
    auto d_t = d_t_ret.value();
    CHECK(d_t.test_oneof.index() == 2);
    CHECK(std::get<2>(d_t.test_oneof) == "oneof index 2");
  }
  SUBCASE("submessage index 3") {
    SampleMessageOneof pb_t;
    auto m = new SubMessageForOneof();
    m->set_ok(true);
    pb_t.set_allocated_sub_message(m);
    auto pb_buf = pb_t.SerializeAsString();

    sample_message_oneof t;
    auto sub = sub_message_for_oneof{true};
    t.test_oneof.emplace<3>(sub);
    auto size = get_needed_size(t);
    REQUIRE(size == pb_buf.size());
    auto buf = serialize<std::string>(t);
    REQUIRE(buf == pb_buf);
    std::size_t len = 0;
    auto d_t_ret = deserialize<sample_message_oneof>(buf.data(), buf.size(), len);
    REQUIRE(d_t_ret);
    REQUIRE(len == buf.size());
    auto d_t = d_t_ret.value();
    CHECK(d_t.test_oneof.index() == 3);
    CHECK(std::get<3>(d_t.test_oneof) == sub);
  }
}