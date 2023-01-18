#pragma once
#include <google/protobuf/compiler/code_generator.h>
namespace struct_pb {
namespace compiler {

class StructGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
  bool Generate(const google::protobuf::FileDescriptor *file,
                const std::string &parameter,
                google::protobuf::compiler::GeneratorContext *generator_context,
                std::string *error) const override;
};

}  // namespace compiler
}  // namespace struct_pb
