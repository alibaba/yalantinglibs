#include "src/struct_pack/benchmark/config.hpp"
#include "src/struct_pack/benchmark/struct_pb_sample.hpp"
#include "struct_pack/struct_pack/pb.hpp"
using namespace struct_pack::pb;
struct my_test_field_number_random {
  varint32_t a;
  sint64_t b;
  std::string c;
  double d;
  float e;
  std::vector<uint32_t> f;
  bool operator==(const my_test_field_number_random&) const = default;
};
template <>
constexpr field_number_array_t<my_test_field_number_random>
    field_number_seq<my_test_field_number_random>{6, 3, 4, 5, 1, 128};
int main(int argc, char** argv) {
  auto ms = struct_pb_sample::create_monsters(OBJECT_COUNT);
  for (int i = 0; i < 10000; ++i) {
    auto buf = struct_pack::pb::serialize(ms);
    std::size_t len = 0;
    struct_pack::pb::deserialize<struct_pb_sample::Monsters>(buf.data(),
                                                             buf.size(), len);
  }
  //  std::vector<int> v;
  //  v.reserve()
  //  v.resize(100);
  //  using T = my_test_field_number_random;
  //  constexpr auto map =
  //  struct_pack::pb::create_map<my_test_field_number_random>(); for(auto e:
  //  map.i2n) {
  //    std::cout << e.first << " " << e.second << std::endl;
  //  }
  //  std::cout << "---------------" << std::endl;
  //  for(auto e: map.n2i) {
  //    std::cout << e.first << " " << e.second << std::endl;
  //  }
  //  constexpr auto i = struct_pack::pb::get_field_index<T>(128);
  //  constexpr auto Count = struct_pack::detail::member_count<T>();
  //  static_assert(i != Count);
  //  std::cout << "index: " << i << std::endl;
  //  constexpr auto n = struct_pack::pb::get_field_number<T>(5);
  //  static_assert(n != 0);
  //  std::cout << "num: " << n << std::endl;

  return 0;
}