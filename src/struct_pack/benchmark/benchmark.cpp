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
  sample&& sp = struct_pack_sample();
  sp.create_samples();
  sp.do_serialization(0);
  sp.do_deserialization(0);

  sp.print_buffer_size();
}

template <SampleType sample_type>
void bench_struct_pb() {
  std::string tag = get_bench_name(sample_type);
  bench(tag, struct_pb_sample::create_sample<sample_type>()
#ifdef HAVE_PROTOBUF
                 ,
        protobuf_sample::create_sample<sample_type>()
#endif
  );
}

void bench_all_struct_pb() {
  bench_struct_pb<SampleType::MONSTER>();
  bench_struct_pb<SampleType::MONSTERS>();
}
int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;
  std::cout << "SAMPLES_COUNT: " << SAMPLES_COUNT << std::endl;

  bench_struct_pack();
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
