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

  return 0;
}
