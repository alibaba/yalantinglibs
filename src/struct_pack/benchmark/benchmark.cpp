#include <memory>
#include <vector>

#include "struct_pack/struct_pack/pb.hpp"
#include "struct_pack_sample.hpp"
#include "struct_pb_sample.hpp"
#ifdef HAVE_MSGPACK
#include "msgpack_sample.hpp"
#endif
#ifdef HAVE_PROTOBUF
#include "protobuf_sample.hpp"
#endif

#include <string>

#include "config.hpp"
using namespace std::string_literals;

void calculate_ser_rate(const auto& vec, SampleType type) {
  auto base = vec[0]->get_ser_time_elapsed_map()[type];

  for (int i = 1; i < vec.size(); ++i) {
    auto ns = vec[i]->get_ser_time_elapsed_map()[type];
    std::cout << "struct_pack serialize " << get_bench_name(type) << " is ["
              << double(ns) / base << "] times faster than " << vec[i]->name()
              << ", [" << base << ", " << ns << "]"
              << "\n";
  }

  std::cout << "========================\n";
}

void calculate_deser_rate(const auto& vec, SampleType type) {
  auto base = vec[0]->get_deser_time_elapsed_map()[type];

  for (int i = 1; i < vec.size(); ++i) {
    auto ns = vec[i]->get_deser_time_elapsed_map()[type];
    std::cout << "struct_pack deserialize " << get_bench_name(type) << " is ["
              << double(ns) / base << "] times faster than " << vec[i]->name()
              << ", [" << base << ", " << ns << "]"
              << "\n";
  }

  std::cout << "========================\n";
}

int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;

  std::vector<std::shared_ptr<base_sample>> vec;
  vec.emplace_back(new struct_pack_sample());

#ifdef HAVE_MSGPACK
  vec.emplace_back(new message_pack_sample());
#endif
#ifdef HAVE_PROTOBUF
  vec.emplace_back(new protobuf_sample_t());
#endif
  vec.emplace_back(new struct_pb_sample_t());

  for (auto sample : vec) {
    std::cout << "======= bench " << sample->name() << "=======\n";
    sample->create_samples();
    sample->do_serialization(0);
    sample->do_deserialization(0);

    sample->print_buffer_size();
  }

  std::cout << "calculate serialization rate\n";
  calculate_ser_rate(vec, SampleType::RECTS);
  calculate_ser_rate(vec, SampleType::PERSONS);
  calculate_ser_rate(vec, SampleType::MONSTERS);

  calculate_deser_rate(vec, SampleType::RECTS);
  calculate_deser_rate(vec, SampleType::PERSONS);
  calculate_deser_rate(vec, SampleType::MONSTERS);

  return 0;
}
