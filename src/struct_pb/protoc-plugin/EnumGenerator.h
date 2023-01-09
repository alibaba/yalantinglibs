#pragma once
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

#include "GeneratorBase.h"
#include "google/protobuf/io/printer.h"
namespace struct_pb {
namespace compiler {
class EnumGenerator : public GeneratorBase {
 public:
  EnumGenerator(const google::protobuf::EnumDescriptor *d, Options options)
      : GeneratorBase(options), d_(d) {}
  void generate(google::protobuf::io::Printer *p);
  void generate_definition(google::protobuf::io::Printer *p);

 private:
  const google::protobuf::EnumDescriptor *d_;
};
}  // namespace compiler
}  // namespace struct_pb