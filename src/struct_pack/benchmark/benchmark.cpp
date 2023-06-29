#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "struct_pack_sample.hpp"

#if __has_include(<msgpack.hpp>)
#define HAVE_MSGPACK 1
#endif

#ifdef HAVE_MSGPACK
#include "msgpack_sample.hpp"
#endif

#ifdef HAVE_PROTOBUF
#include "protobuf_sample.hpp"
#include "struct_pb_sample.hpp"
#endif
#ifdef HAVE_FLATBUFFER
#include "flatbuffer_sample.hpp"
#endif

#include "config.hpp"
using namespace std::string_literals;

void calculate_ser_rate(const auto& map, LibType base_line_type,
                        SampleType sample_type) {
  auto it = map.find(base_line_type);
  if (it == map.end()) {
    throw std::invalid_argument("unknown lib type");
  }

  auto base_lib = it->second;
  auto base_line_ser = base_lib->get_ser_time_elapsed_map()[sample_type];
  auto base_line_deser = base_lib->get_deser_time_elapsed_map()[sample_type];

  for (auto [lib_type, other_lib] : map) {
    if (lib_type == base_line_type) {
      continue;
    }

    auto ns_ser = other_lib->get_ser_time_elapsed_map()[sample_type];
    auto ns_deser = other_lib->get_deser_time_elapsed_map()[sample_type];
    double rate_ser = std::ceil(double(ns_ser) / base_line_ser * 100.0) / 100.0;
    double rate_deser =
        std::ceil(double(ns_deser) / base_line_deser * 100.0) / 100.0;

    std::string prefix_ser = base_lib->name()
                                 .append(" serialize   ")
                                 .append(get_sample_name(sample_type))
                                 .append(" is ");
    std::string prefix_deser = base_lib->name()
                                   .append(" deserialize ")
                                   .append(get_sample_name(sample_type))
                                   .append(" is ");

    auto space_ser = get_space_str(prefix_ser.size(), 39);
    auto space_deser = get_space_str(prefix_deser.size(), 39);

    std::cout << prefix_ser << space_ser << "[" << rate_ser
              << "] times faster than " << other_lib->name() << ", ["
              << base_line_ser << ", " << ns_ser << "]"
              << "\n";
    std::cout << prefix_deser << space_deser << "[" << rate_deser
              << "] times faster than " << other_lib->name() << ", ["
              << base_line_deser << ", " << ns_deser << "]"
              << "\n";
  }

  std::cout << "========================\n";
}

void run_benchmark(const auto& map, LibType base_line_type) {
  std::cout << "======== calculate serialization rate ========\n";
  calculate_ser_rate(map, base_line_type, SampleType::RECT);
  calculate_ser_rate(map, base_line_type, SampleType::RECTS);
  calculate_ser_rate(map, base_line_type, SampleType::PERSON);
  calculate_ser_rate(map, base_line_type, SampleType::PERSONS);
  calculate_ser_rate(map, base_line_type, SampleType::MONSTER);
  calculate_ser_rate(map, base_line_type, SampleType::MONSTERS);
}

int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;

  std::map<LibType, std::shared_ptr<base_sample>> map;
  map.emplace(LibType::STRUCT_PACK, new struct_pack_sample());

#ifdef HAVE_MSGPACK
  map.emplace(LibType::MSGPACK, new message_pack_sample());
#endif
#ifdef HAVE_PROTOBUF
  map.emplace(LibType::STRUCT_PB, new struct_pb_sample::struct_pb_sample_t());
  map.emplace(LibType::PROTOBUF, new protobuf_sample_t());
#endif
#ifdef HAVE_FLATBUFFER
  map.emplace(LibType::FLATBUFFER, new flatbuffer_sample_t());
#endif

  for (auto [lib_type, sample] : map) {
    std::cout << "======= bench " << sample->name() << "=======\n";
    sample->create_samples();
    sample->do_serialization();
    sample->do_deserialization();
  }

  std::cout << "======= serialize buffer size ========\n";
  for (auto [lib_type, sample] : map) {
    sample->print_buffer_size(lib_type);
  }

  run_benchmark(map, LibType::STRUCT_PACK);

#ifdef HAVE_PROTOBUF

  run_benchmark(map, LibType::STRUCT_PB);
#endif

  return 0;
}
