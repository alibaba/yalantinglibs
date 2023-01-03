#pragma once
#include "Options.hpp"
#include "google/protobuf/descriptor.h"
namespace struct_pb {
namespace compiler {
using namespace google::protobuf;
class GeneratorBase {
 public:
  GeneratorBase(Options options) : options_(options) {}
  std::string get_type(google::protobuf::FieldDescriptor::Type t,
                       const std::string &name = "");
  std::string get_type_str(const google::protobuf::FieldDescriptor *d);
  std::string get_field_type_str(const google::protobuf::FieldDescriptor *d);
  std::string get_prefer_struct_name(const std::string &name) const;
  std::string get_prefer_field_name(const std::string &name) const;
  std::string add_modifier(const google::protobuf::FieldDescriptor *f,
                           std::string type_name) const;

 protected:
  Options options_;
};
}  // namespace compiler
}  // namespace struct_pb
