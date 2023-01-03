#pragma once
#include "GeneratorBase.h"
#include "helpers.hpp"
namespace struct_pb {
namespace compiler {
class OneofGenerator;
bool is_varint(const FieldDescriptor *f);
bool is_sint(const FieldDescriptor *f);
bool is_sint32(const FieldDescriptor *f);
bool is_bool(const FieldDescriptor *f);
bool is_i64(const FieldDescriptor *f);
bool is_i32(const FieldDescriptor *f);
uint32_t calculate_tag(const FieldDescriptor *f, bool ignore_repeated = false);
std::string calculate_tag_str(const FieldDescriptor *f,
                              bool ignore_repeated = false);
std::string calculate_tag_size(const FieldDescriptor *f,
                               bool ignore_repeated = false);
class FieldGenerator : public GeneratorBase {
 public:
  FieldGenerator(const google::protobuf::FieldDescriptor *d,
                 const Options &options)
      : GeneratorBase(options), d_(d) {}
  void generate_definition(google::protobuf::io::Printer *p) const;
  void generate_calculate_size(google::protobuf::io::Printer *p,
                               bool can_ignore_default_value) const;
  void generate_serialization(google::protobuf::io::Printer *p,
                              bool can_ignore_default_value) const;
  void generate_deserialization(google::protobuf::io::Printer *p) const;

  virtual void generate_calculate_size(google::protobuf::io::Printer *p,
                                       const std::string &value,
                                       bool can_ignore_default_value) const;
  virtual void generate_serialization(google::protobuf::io::Printer *p,
                                      const std::string &value,
                                      bool can_ignore_default_value) const;
  virtual void generate_deserialization(google::protobuf::io::Printer *p,
                                        const std::string &value) const;
  virtual std::string cpp_type_name() const;
  virtual std::string pb_type_name() const;

 protected:
  std::string get_type_name() const;
  bool is_ptr() const;
  std::string name() const;
  bool is_optional() const;
  std::string qualified_name() const;

 protected:
  const google::protobuf::FieldDescriptor *d_;
  friend class OneofGenerator;
};
class OneofGenerator : public GeneratorBase {
 public:
  OneofGenerator(const google::protobuf::OneofDescriptor *d,
                 const Options &options)
      : GeneratorBase(options), d_(d) {}
  void generate_definition(google::protobuf::io::Printer *p);
  void generate_calculate_size(google::protobuf::io::Printer *p);
  void generate_serialization(google::protobuf::io::Printer *p,
                              const google::protobuf::FieldDescriptor *f);
  void generate_deserialization(google::protobuf::io::Printer *p,
                                const google::protobuf::FieldDescriptor *f);

 private:
  int get_index(const google::protobuf::FieldDescriptor *f) const;
  std::string name() const;

 private:
  const google::protobuf::OneofDescriptor *d_;
};
class MapFieldGenerator;
class FieldGeneratorMap {
 public:
  FieldGeneratorMap(const Descriptor *descriptor, const Options &options);
  FieldGeneratorMap(const FieldGeneratorMap &) = delete;
  FieldGeneratorMap &operator=(const FieldGeneratorMap &) = delete;

  const FieldGenerator &get(const FieldDescriptor *field) const;

 private:
  const Descriptor *descriptor_;
  std::vector<std::unique_ptr<FieldGenerator>> field_generators_;

  static FieldGenerator *MakeGenerator(const FieldDescriptor *field,
                                       const Options &options);
  friend class MapFieldGenerator;
};

}  // namespace compiler
}  // namespace struct_pb