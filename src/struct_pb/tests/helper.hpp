#pragma once
#include "struct_pack/struct_pack.hpp"
#include "struct_pb/struct_pb.hpp"
#include "test_pb.struct_pb.h"
#include "hex_printer.hpp"
#ifdef HAVE_PROTOBUF
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#endif
template <typename Buffer, typename T>
std::string serialize(const T& t) {
  std::string buffer;
  auto size = struct_pb::internal::get_needed_size(t);
  buffer.resize(size);
  struct_pb::internal::serialize_to(buffer.data(), buffer.size(), t);
  return buffer;
}
template <typename T>
struct_pack::expected<T, struct_pack::errc> deserialize(const char* data,
                                                        std::size_t size,
                                                        std::size_t& len) {
  len = size;
  struct_pack::expected<T, struct_pack::errc> ret;
  bool ok = struct_pb::internal::deserialize_to(ret.value(), data, size);
  if (ok) {
    return ret;
  }
  else {
    ret = struct_pack::unexpected<struct_pack::errc>{
        struct_pack::errc::invalid_argument};
    return ret;
  }
}

template <typename T, typename Pred = std::equal_to<T>>
void check_self(const T& t, std::optional<std::string> buf_opt = {}) {
  auto size = struct_pb::internal::get_needed_size(t);
  if (buf_opt.has_value()) {
    REQUIRE(buf_opt.value().size() == size);
  }
  std::string b;
  b.resize(size);
  struct_pb::internal::serialize_to(b.data(), b.size(), t);
  if (buf_opt.has_value()) {
    CHECK(hex_helper(buf_opt.value()) == hex_helper(b));
  }
  T d_t{};
  auto ok = struct_pb::internal::deserialize_to(d_t, b.data(), b.size());
  REQUIRE(ok);
  CHECK(Pred()(t, d_t));
}

#ifdef HAVE_PROTOBUF
template <typename T, typename PB_T>
struct PB_equal : public std::binary_function<T, PB_T, bool> {
  constexpr bool operator()(const T& t, const PB_T& pb_t) const { return t == pb_t; }
};
template <typename T, typename PB_T, typename Pred = PB_equal<T, PB_T>>
void check_with_protobuf(const T& t, const PB_T& pb_t) {
  auto pb_buf = pb_t.SerializeAsString();
  auto size = struct_pb::internal::get_needed_size(t);
  REQUIRE(size == pb_buf.size());

  std::string b;
  b.resize(size);
  struct_pb::internal::serialize_to(b.data(), b.size(), t);
  CHECK(hex_helper(b) == hex_helper(pb_buf));

  CHECK(Pred()(t, pb_t));
  T d_t{};
  auto ok = struct_pb::internal::deserialize_to(d_t, b.data(), b.size());
  REQUIRE(ok);
  CHECK(Pred()(d_t, pb_t));
}
template <typename T, typename U>
bool operator==(const std::vector<T>& a,
                const google::protobuf::RepeatedPtrField<U>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  auto sz = a.size();
  for (int i = 0; i < sz; ++i) {
    if (!PB_equal<T, U>()(a[i], b.at(i))) {
      return false;
    }
  }
  return true;
}
template <typename T, typename U>
bool operator==(const std::vector<T>& a,
                const google::protobuf::RepeatedField<U>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  auto sz = a.size();
  for (int i = 0; i < sz; ++i) {
    if (a[i] != b.at(i)) {
      return false;
    }
  }
  return true;
}
#endif