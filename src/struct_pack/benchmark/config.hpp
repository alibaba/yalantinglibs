#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
static constexpr int OBJECT_COUNT = 10000;

constexpr static std::size_t RUN_COUNT = 10;
enum class LibType {
  STRUCT_PACK,
  STRUCT_PB,
  MSGPACK,
  PROTOBUF,
};
enum class SampleType { RECT, RECTS, PERSON, PERSONS, MONSTER, MONSTERS };

inline const std::unordered_map<SampleType, std::string> g_sample_name_map = {
    {SampleType::RECT, "1 rect"},       {SampleType::RECTS, "many rects"},
    {SampleType::PERSON, "1 person"},   {SampleType::PERSONS, "many persons"},
    {SampleType::MONSTER, "1 monster"}, {SampleType::MONSTERS, "many monsters"},
};

inline const std::vector<SampleType> g_sample_type_vec = {
    SampleType::RECT,    SampleType::RECTS,   SampleType::PERSON,
    SampleType::PERSONS, SampleType::MONSTER, SampleType::MONSTERS};

inline std::string get_bench_name(SampleType sample_type) {
  auto it = g_sample_name_map.find(sample_type);
  if (it == g_sample_name_map.end()) {
    throw std::invalid_argument("unknown sample type");
  }

  return it->second;
}
