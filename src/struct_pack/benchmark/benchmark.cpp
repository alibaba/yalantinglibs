#include "struct_pack_sample.hpp"
#ifdef HAVE_MSGPACK
#include "msgpack_sample.hpp"
#endif
#ifdef HAVE_PROTOBUF
#include "protobuf_sample.hpp"
#endif
#ifdef HAVE_PROTOPUF
#include "protopuf_sample.hpp"
#endif
#ifdef HAVE_PROTOZERO
#include "protozero_sample.hpp"
#endif

#include <string>

#include "bench.hpp"
#include "config.hpp"
using namespace std::string_literals;

template <SampleType sample_type>
void bench_struct_pack() {
  std::string tag = get_tag_name<sample_type>();
  bench(tag, create_sample<sample_type>()
#ifdef HAVE_MSGPACK
                 ,
        msgpack_sample::create_sample<sample_type>()
#endif
#ifdef HAVE_PROTOBUF
            ,
        protobuf_sample::create_sample<sample_type>()
#endif
#ifdef HAVE_PROTOPUF
            ,
        protopuf_sample::create_sample<sample_type>()
#endif
#ifdef HAVE_PROTOZERO
            ,
        protozero_sample::create_sample<sample_type>()
#endif
  );
}

void bench_all_struct_pack() {
  bench_struct_pack<SampleType::MONSTER>();
  bench_struct_pack<SampleType::MONSTERS>();
}
int main(int argc, char** argv) {
  std::cout << "OBJECT_COUNT : " << OBJECT_COUNT << std::endl;
  std::cout << "SAMPLES_COUNT: " << SAMPLES_COUNT << std::endl;

  bench_all_struct_pack();
  return 0;
}
