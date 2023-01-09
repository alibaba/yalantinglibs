#pragma once
#include "FieldGenerator.h"
#include "google/protobuf/descriptor.h"
namespace struct_pb {
namespace compiler {
using namespace google::protobuf;
class PrimitiveFieldGenerator : public FieldGenerator {
 public:
  PrimitiveFieldGenerator(const FieldDescriptor *descriptor,
                          const Options &options);
  PrimitiveFieldGenerator(const PrimitiveFieldGenerator &) = delete;
  PrimitiveFieldGenerator &operator=(const PrimitiveFieldGenerator &) = delete;
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
  void generate_deserialization_only(
      google::protobuf::io::Printer *p, const std::string &output,
      const std::string &max_size = "size") const;
};
class RepeatedPrimitiveFieldGenerator : public FieldGenerator {
 public:
  RepeatedPrimitiveFieldGenerator(const FieldDescriptor *descriptor,
                                  const Options &options);
  std::string cpp_type_name() const override;
  void generate_calculate_size(
      google::protobuf::io::Printer *p, const std::string &value = {},
      bool can_ignore_default_value = true) const override;
  void generate_calculate_packed_size_only(google::protobuf::io::Printer *p,
                                           const std::string &value) const;
  void generate_calculate_unpacked_size_only(google::protobuf::io::Printer *p,
                                             const std::string &value) const;
  void generate_serialization(google::protobuf::io::Printer *p,
                              const std::string &value,
                              bool can_ignore_default_value) const override;
  void generate_deserialization(google::protobuf::io::Printer *p,
                                const std::string &value) const override;
  void generate_deserialization_packed_only(google::protobuf::io::Printer *p,
                                            const std::string &value,
                                            const std::string &max_size) const;
  void generate_deserialization_unpacked_only(google::protobuf::io::Printer *p,
                                              const std::string &output) const;

 private:
  bool is_packed() const;
  std::string packed_tag() const;
  std::string unpacked_tag() const;
};
}  // namespace compiler
}  // namespace struct_pb
