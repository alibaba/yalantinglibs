#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
static constexpr int OBJECT_COUNT = 20;
#ifdef NDEBUG
static constexpr int SAMPLES_COUNT = 10000;  // 0;
#else
static constexpr int SAMPLES_COUNT = 100;
#endif
constexpr static std::size_t RUN_COUNT = 10;
enum class LibType {
  STRUCT_PACK,
  STRUCT_PB,
  MSGPACK,
  PROTOBUF,
};
enum class SampleType { RECT, RECTS, PERSON, PERSONS, MONSTER, MONSTERS };

inline const std::unordered_map<SampleType, std::string> sample_name_map = {
    {SampleType::RECT, "1 rect"},       {SampleType::RECTS, "20 rects"},
    {SampleType::PERSON, "1 person"},   {SampleType::PERSONS, "20 persons"},
    {SampleType::MONSTER, "1 monster"}, {SampleType::MONSTERS, "20 monsters"},
};

inline std::string get_tag_name(SampleType sample_type) {
  auto it = sample_name_map.find(sample_type);
  if (it == sample_name_map.end()) {
    throw std::invalid_argument("unknown sample type");
  }

  return it->second;
}
