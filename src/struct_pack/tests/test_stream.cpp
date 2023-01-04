#include <fstream>
#include <ios>
#include <iostream>

#include "doctest.h"
#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/error_code.h"
#include "test_struct.hpp"

TEST_CASE("test serialize_to/deserialize file") {
  person p1{.age = 24, .name = "Betty"}, p2{.age = 45, .name = "Tom"};
  complicated_object v{
      .color = Color::red,
      .a = 42,
      .b = "hello",
      .c = {{20, "tom"}, {22, "jerry"}},
      .d = {"hello", "world"},
      .e = {1, 2},
      .f = {{1, {20, "tom"}}},
      .g = {{1, {20, "tom"}}, {1, {22, "jerry"}}},
      .h = {"aa", "bb"},
      .i = {1, 2},
      .j = {{1, {20, "tom"}}, {1, {22, "jerry"}}},
      .k = {{1, 2}, {1, 3}},
      .m = {person{20, "tom"}, {22, "jerry"}},
      .n = {person{20, "tom"}, {22, "jerry"}},
      .o = std::make_pair("aa", person{20, "tom"}),
  };
  nested_object nested{.id = 2, .name = "tom", .p = {20, "tom"}, .o = v};
  SUBCASE("serialize_to empty file") {
    // serialize to file
    std::ofstream ofs("1.save", std::ofstream::binary | std::ofstream::out);
    for (int i = 0; i < 100; ++i) {
      struct_pack::serialize_to(ofs, p1);
      struct_pack::serialize_to(ofs, nested);
    }
    auto total_size = (struct_pack::get_needed_size(p1) +
                       struct_pack::get_needed_size(nested)) *
                      100;
    CHECK(ofs.tellp() == total_size);
  }
  SUBCASE("serialize_to non-empty file") {
    // serialize to file
    std::ofstream ofs("1.save", std::ofstream::binary | std::ofstream::out |
                                    std::ostream::app);
    for (int i = 0; i < 100; ++i) {
      struct_pack::serialize_to(ofs, p2);
      struct_pack::serialize_to(ofs, nested);
    }
  }
  SUBCASE("test deserialize") {
    std::ifstream ifs("1.save", std::ofstream::binary | std::ofstream::in);
    for (int i = 0; i < 100; ++i) {
      auto person1 = struct_pack::deserialize<person>(ifs);
      CHECK(person1 == p1);
      auto nested1 = struct_pack::deserialize<nested_object>(ifs);
      CHECK(nested1 == nested);
    }
    for (int i = 0; i < 100; ++i) {
      auto person2 = struct_pack::deserialize<person>(ifs);
      CHECK(person2 == p2);
      auto nested1 = struct_pack::deserialize<nested_object>(ifs);
      CHECK(nested1 == nested);
    }
  }
  SUBCASE("test deserialize_to") {
    std::ifstream ifs("1.save", std::ofstream::binary | std::ofstream::in);
    for (int i = 0; i < 100; ++i) {
      person person1;
      auto ec = struct_pack::deserialize_to(person1, ifs);
      CHECK(ec == struct_pack::errc{});
      CHECK(person1 == p1);
      nested_object nested1;
      ec = struct_pack::deserialize_to(nested1, ifs);
      CHECK(ec == struct_pack::errc{});
      CHECK(nested1 == nested);
    }
    for (int i = 0; i < 100; ++i) {
      person person2;
      auto ec = struct_pack::deserialize_to(person2, ifs);
      CHECK(ec == struct_pack::errc{});
      CHECK(person2 == p2);
      nested_object nested1;
      ec = struct_pack::deserialize_to(nested1, ifs);
      CHECK(ec == struct_pack::errc{});
      CHECK(nested1 == nested);
    }
  }
}

TEST_CASE("test get_field file") {
  std::array<person, 2> p{person{.age = 24, .name = "Betty"},
                          person{.age = 45, .name = "Tom"}};
  std::ofstream ofs("2.save", std::ofstream::binary | std::ofstream::out);
  for (int i = 0; i < 100; ++i) {
    for (auto &p1 : p) {
      auto pos = ofs.tellp();
      CHECK(ofs.seekp(4, std::ios_base::cur));
      struct_pack::serialize_to(ofs, p1);
      auto npos = ofs.tellp();
      CHECK(ofs.seekp(pos));
      uint32_t sz = npos - pos;
      CHECK(ofs.write((char *)&sz, 4));
      CHECK(ofs.seekp(npos));
    }
  }
  auto total_size = (struct_pack::get_needed_size(p[0]) +
                     struct_pack::get_needed_size(p[1]) + 8) *
                    100;
  CHECK(ofs.tellp() == total_size);

  SUBCASE("test get_field") {
    std::ifstream ifs("2.save", std::ofstream::binary | std::ofstream::in);
    for (int i = 0; i < 100; ++i) {
      for (auto &p1 : p) {
        std::size_t pos = ifs.tellg();
        std::uint32_t sz;
        CHECK(ifs.read((char *)&sz, 4));
        auto name = struct_pack::get_field<person, 1>(ifs);
        CHECK(name == p1.name);
        CHECK(ifs.seekg(pos + sz));
      }
    }
  }
  SUBCASE("test get_field") {
    std::ifstream ifs("2.save", std::ofstream::binary | std::ofstream::in);
    for (int i = 0; i < 100; ++i) {
      for (auto &p1 : p) {
        std::size_t pos = ifs.tellg();
        std::uint32_t sz;
        CHECK(ifs.read((char *)&sz, 4));
        std::string name;
        auto ec = struct_pack::get_field_to<person, 1>(name, ifs);
        CHECK(ec == struct_pack::errc{});
        CHECK(name == p1.name);
        CHECK(ifs.seekg(pos + sz));
      }
    }
  }
}

TEST_CASE("test compatible obj") {
  person p{.age = 24, .name = "Betty"};
  person1 p1{.age = 24, .name = "Betty"};
  person1 p2{.age = 24, .name = "Betty", .id = 114514};
  person1 p3{.age = 24, .name = "Betty", .id = 114514, .maybe = true};
  SUBCASE("forward") {
    {
      std::ofstream ofs("3.save", std::ofstream::binary | std::ofstream::out);
      for (int i = 0; i < 100; ++i) {
        struct_pack::serialize_to(ofs, p1);
        struct_pack::serialize_to(ofs, p2);
        struct_pack::serialize_to(ofs, p3);
      }
      auto total_size =
          (struct_pack::get_needed_size(p1) + struct_pack::get_needed_size(p2) +
           struct_pack::get_needed_size(p3)) *
          100;
      CHECK(ofs.tellp() == total_size);
    }
    {
      std::ifstream ifs("3.save", std::ofstream::binary | std::ofstream::in);
      for (int i = 0; i < 100; ++i) {
        auto person1 = struct_pack::deserialize<person>(ifs);
        CHECK(person1 == p);
        auto person2 = struct_pack::deserialize<person>(ifs);
        CHECK(person2 == p);
        auto person3 = struct_pack::deserialize<person>(ifs);
        CHECK(person3 == p);
      }
    }
  }
  SUBCASE("backward") {
    {
      std::ofstream ofs("4.save", std::ofstream::binary | std::ofstream::out);
      for (int i = 0; i < 100; ++i) {
        struct_pack::serialize_to(ofs, p);
        struct_pack::serialize_to(ofs, p1);
        struct_pack::serialize_to(ofs, p2);
      }
      auto total_size =
          (struct_pack::get_needed_size(p) + struct_pack::get_needed_size(p1) +
           struct_pack::get_needed_size(p2)) *
          100;
      CHECK(ofs.tellp() == total_size);
    }
    {
      std::ifstream ifs("4.save", std::ofstream::binary | std::ofstream::in);
      for (int i = 0; i < 100; ++i) {
        auto res1 = struct_pack::deserialize<person1>(ifs);
        CHECK(res1 == p1);
        auto res2 = struct_pack::deserialize<person1>(ifs);
        CHECK(res2 == p1);
        auto res3 = struct_pack::deserialize<person1>(ifs);
        CHECK(res3 == p2);
      }
    }
    {
      std::ifstream ifs("4.save", std::ofstream::binary | std::ofstream::in);
      auto res = struct_pack::get_field<person1, 2>(ifs);
      CHECK(res.has_value() == true);
      CHECK(*res == std::nullopt);
    }
  }
}