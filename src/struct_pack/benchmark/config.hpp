#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
inline constexpr int OBJECT_COUNT = 20;
inline constexpr int ITERATIONS = 100000;

enum class LibType {
  STRUCT_PACK,
  STRUCT_PB,
  MSGPACK,
  PROTOBUF,
  FLATBUFFER,
};
enum class SampleType { RECT, RECTS, PERSON, PERSONS, MONSTER, MONSTERS };

inline const std::unordered_map<SampleType, std::string> g_sample_name_map = {
    {SampleType::RECT, "1 rect"},
    {SampleType::RECTS, std::to_string(OBJECT_COUNT) + " rects"},
    {SampleType::PERSON, "1 person"},
    {SampleType::PERSONS, std::to_string(OBJECT_COUNT) + " persons"},
    {SampleType::MONSTER, "1 monster"},
    {SampleType::MONSTERS, std::to_string(OBJECT_COUNT) + " monsters"},
};

inline const std::unordered_map<LibType, std::string> g_lib_name_map = {
    {LibType::STRUCT_PACK, "struct_pack"},
    {LibType::STRUCT_PB, "struct_pb"},
    {LibType::MSGPACK, "msgpack"},
    {LibType::PROTOBUF, "protobuf"},
    {LibType::FLATBUFFER, "flatbuffer"}};

inline const std::vector<SampleType> g_sample_type_vec = {
    SampleType::RECT,    SampleType::RECTS,   SampleType::PERSON,
    SampleType::PERSONS, SampleType::MONSTER, SampleType::MONSTERS};

inline std::string get_sample_name(SampleType sample_type) {
  auto it = g_sample_name_map.find(sample_type);
  if (it == g_sample_name_map.end()) {
    throw std::invalid_argument("unknown sample type");
  }

  return it->second;
}

inline std::string get_lib_name(LibType lib_type) {
  auto it = g_lib_name_map.find(lib_type);
  if (it == g_lib_name_map.end()) {
    throw std::invalid_argument("unknown lib type");
  }

  return it->second;
}

inline std::string get_space_str(size_t size, size_t max_size) {
  std::string space_str = "";
  if (size < max_size) {
    for (std::size_t i = 0; i < max_size - size; ++i) space_str.append(" ");
  }
  return space_str;
}
