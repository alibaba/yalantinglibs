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

#include "bench.hpp"
#include "config.hpp"
using namespace std::string_literals;

void bench_struct_pack() {
  base_sample&& sp = struct_pack_sample();
  sp.create_samples();
  sp.do_serialization(0);
  sp.do_deserialization(0);

  sp.print_buffer_size();
}

void bench_all_struct_pb() {
  // bench_struct_pb<SampleType::MONSTER>();
  // bench_struct_pb<SampleType::MONSTERS>();
}
int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;
  std::cout << "SAMPLES_COUNT: " << SAMPLES_COUNT << std::endl;

  std::vector<std::shared_ptr<base_sample>> vec;
  vec.emplace_back(new struct_pack_sample());

#ifdef HAVE_MSGPACK
  vec.emplace_back(new message_pack_sample());
#endif
#ifdef HAVE_PROTOBUF
  vec.emplace_back(new protobuf_sample_t());
#endif

  for (auto sample : vec) {
    std::cout << "======= bench " << sample->name() << "=======\n";
    sample->create_samples();
    sample->do_serialization(0);
    sample->do_deserialization(0);

    // sample->print_buffer_size();
  }

  // bench_struct_pack();
  return 0;

  if (argc == 1) {
    bench_all_struct_pb();
    return 0;
  }

  else {
    bench_all_struct_pb();
  }
  return 0;
}
