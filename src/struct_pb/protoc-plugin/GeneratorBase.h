#pragma once
#include "Options.hpp"
#include "google/protobuf/descriptor.h"
namespace struct_pb {
namespace compiler {
using namespace google::protobuf;
class GeneratorBase {
 public:
  GeneratorBase(Options options) : options_(options) {}

 protected:
  Options options_;
};
}  // namespace compiler
}  // namespace struct_pb
