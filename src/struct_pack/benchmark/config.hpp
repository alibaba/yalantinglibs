#pragma once
#include <cstdint>
static constexpr int OBJECT_COUNT = 20;
#ifdef NDEBUG
static constexpr int SAMPLES_COUNT = 10000;  // 0;
#else
static constexpr int SAMPLES_COUNT = 100;
#endif
constexpr static std::size_t RUN_COUNT = 10;
enum class SampleName {
  STRUCT_PACK,
  STRUCT_PB,
  MSGPACK,
  PROTOBUF,
  PROTOPUF,
  PROTOZERO
};
enum class SampleType { RECT, RECTS, PERSON, PERSONS, MONSTER, MONSTERS };

template <SampleType sample_type>
inline auto get_tag_name() {
  if constexpr (sample_type == SampleType::RECT) {
    return "1 rect";
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    return "20 rects";
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    return "1 person";
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    return "20 persons";
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    return "1 monster";
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    return "20 monsters";
  }
  else {
    return "";
  }
}
