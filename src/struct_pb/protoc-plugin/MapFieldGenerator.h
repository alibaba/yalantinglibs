#pragma once
#include "FieldGenerator.h"
namespace struct_pb {
namespace compiler {

class MapFieldGenerator : public FieldGenerator {
 public:
  MapFieldGenerator(const FieldDescriptor *field, const Options &options);
  std::string cpp_type_name() const override;
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
  void generate_deserialization_only(google::protobuf::io::Printer *p,
                                     const std::string &value) const;

 private:
  void generate_calculate_kv_size(google::protobuf::io::Printer *p,
                                  const FieldDescriptor *f,
                                  const std::string &value) const;
  void generate_serialize_kv_only(google::protobuf::io::Printer *p,
                                  const FieldDescriptor *f,
                                  const std::string &value) const;
  void generate_deserialize_kv_only(google::protobuf::io::Printer *p,
                                    const FieldDescriptor *f,
                                    const std::string &value,
                                    const std::string &max_size) const;

 private:
  std::string get_key_type_name() const;
  std::string get_value_type_name() const;
  std::string get_kv_type_name_helper(const FieldDescriptor *f) const;
};

}  // namespace compiler
}  // namespace struct_pb
