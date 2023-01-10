#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "struct_pack_sample.hpp"
#ifdef HAVE_MSGPACK
#include "msgpack_sample.hpp"
#endif
#ifdef HAVE_PROTOBUF
#include "protobuf_sample.hpp"
#endif

#include <string>

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
    std::cout << base_lib->name() << " serialize "
              << get_sample_name(sample_type) << " is ["
              << double(ns_ser) / base_line_ser << "] times faster than "
              << other_lib->name() << ", [" << base_line_ser << ", " << ns_ser
              << "]"
              << "\n";
    std::cout << base_lib->name() << " deserialize "
              << get_sample_name(sample_type) << " is ["
              << double(ns_deser) / base_line_deser << "] times faster than "
              << other_lib->name() << ", [" << base_line_deser << ", "
              << ns_deser << "]"
              << "\n";
  }

  std::cout << "========================\n";
}

int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;

  std::unordered_map<LibType, std::shared_ptr<base_sample>> map;
  map.emplace(LibType::STRUCT_PACK, new struct_pack_sample());

#ifdef HAVE_MSGPACK
  map.emplace(LibType::MSGPACK, new message_pack_sample());
#endif
#ifdef HAVE_PROTOBUF
  map.emplace(LibType::PROTOBUF, new protobuf_sample_t());
#endif

  for (auto [lib_type, sample] : map) {
    std::cout << "======= bench " << sample->name() << "=======\n";
    sample->create_samples();
    sample->do_serialization();
    sample->do_deserialization();

    sample->print_buffer_size(lib_type);
  }

  std::cout << "calculate serialization rate\n";
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::RECT);
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::RECTS);
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::PERSON);
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::PERSONS);
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::MONSTER);
  calculate_ser_rate(map, LibType::STRUCT_PACK, SampleType::MONSTERS);

  return 0;
}
