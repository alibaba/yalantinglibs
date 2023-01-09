#pragma once
#include "FieldGenerator.h"
namespace struct_pb {
namespace compiler {

class MessageFieldGenerator : public FieldGenerator {
 public:
  MessageFieldGenerator(const FieldDescriptor *field, const Options &options);
  void generate_calculate_size(google::protobuf::io::Printer *p,
                               const std::string &value,
                               bool can_ignore_default_value) const override;
  void generate_serialization(google::protobuf::io::Printer *p,
                              const std::string &value,
                              bool can_ignore_default_value) const override;
  void generate_deserialization(google::protobuf::io::Printer *p,
                                const std::string &value) const override;
  std::string cpp_type_name() const override;
};
class RepeatedMessageFieldGenerator : public FieldGenerator {
 public:
  RepeatedMessageFieldGenerator(const FieldDescriptor *field,
                                const Options &options);
  void generate_calculate_size(google::protobuf::io::Printer *p,
                               const std::string &value,
                               bool can_ignore_default_value) const override;
  void generate_calculate_size_only(google::protobuf::io::Printer *p,
                                    const std::string &value) const;
  void generate_serialization(google::protobuf::io::Printer *p,
                              const std::string &value,
                              bool can_ignore_default_value) const override;
  void generate_serialization_only(google::protobuf::io::Printer *p,
                                   const std::string &value) const;
  void generate_deserialization(google::protobuf::io::Printer *p,
                                const std::string &value) const override;
  std::string cpp_type_name() const override;
};
}  // namespace compiler
}  // namespace struct_pb
