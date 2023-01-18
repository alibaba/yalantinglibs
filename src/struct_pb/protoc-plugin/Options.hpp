#pragma once
#include <string>

#include "google/protobuf/descriptor.h"
struct Options {
  Options(const google::protobuf::FileDescriptor* f) : f(f) {}
  bool generate_eq_op = false;
  std::string ns;
  const google::protobuf::FileDescriptor* f;
};