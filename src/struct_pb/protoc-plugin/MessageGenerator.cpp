#include "MessageGenerator.h"

#include "EnumGenerator.h"
#include "FieldGenerator.h"
#include "helpers.hpp"
using namespace google::protobuf;
using namespace google::protobuf::internal;
namespace struct_pb {
namespace compiler {
std::unordered_map<std::string_view, std::string> ClassVars(
    const Descriptor *descriptor, Options options) {
  std::unordered_map<std::string_view, std::string> vars{};
  vars.emplace("classname", resolve_keyword(descriptor->name()));
  vars.emplace("classtype", qualified_class_name(descriptor, options));
  return vars;
}
// copy from cpp compiler
static std::string UnderscoresToCamelCase(const std::string &input,
                                          bool cap_next_letter) {
  std::string result;
  // Note:  I distrust ctype.h due to locales.
  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      if (cap_next_letter) {
        result += input[i] + ('A' - 'a');
      }
      else {
        result += input[i];
      }
      cap_next_letter = false;
    }
    else if ('A' <= input[i] && input[i] <= 'Z') {
      // Capital letters are left as-is.
      result += input[i];
      cap_next_letter = false;
    }
    else if ('0' <= input[i] && input[i] <= '9') {
      result += input[i];
      cap_next_letter = true;
    }
    else {
      cap_next_letter = true;
    }
  }
  return result;
}

void MessageGenerator::generate_struct_definition(
    google::protobuf::io::Printer *p) {
  //  auto v = p->WithVars(ClassVars(d_, options_));
  Formatter format(p);
  format("struct $1$ {\n", resolve_keyword(d_->name()));
  format.indent();
  for (int i = 0; i < d_->enum_type_count(); ++i) {
    auto e = d_->enum_type(i);
    EnumGenerator(e, options_).generate_definition(p);
  }
  for (int i = 0; i < d_->oneof_decl_count(); ++i) {
    auto oneof = d_->oneof_decl(i);
    std::string enum_name = UnderscoresToCamelCase(oneof->name(), true);
    format("enum class $1$Case {\n", enum_name);
    format.indent();
    format("none = 0,\n");
    for (int j = 0; j < oneof->field_count(); ++j) {
      auto f = oneof->field(j);
      format("$1$ = $2$,\n", resolve_keyword(f->name()), f->number());
    }
    format.outdent();
    format("};\n");
    format("$1$Case $2$_case() const {\n", enum_name, oneof->name());
    format.indent();
    format("switch ($1$.index()) {\n", resolve_keyword(oneof->name()));
    format.indent();
    for (int j = 0; j < oneof->field_count(); ++j) {
      format("case $1$:\n", j + 1);
      format.indent();
      format("return $1$Case::$2$;\n", enum_name,
             resolve_keyword(oneof->field(j)->name()));
      format.outdent();
    }
    format("default:\n");
    format.indent();
    format("return $1$Case::none;\n", enum_name);
    format.outdent();

    format.outdent();
    format("}\n");
    format.outdent();
    format("}\n");
    format("\n");
  }
  for (int i = 0; i < d_->nested_type_count(); ++i) {
    auto t = d_->nested_type(i);
    auto name = t->name();
    if (is_map_entry(t)) {
      continue;
    }
    MessageGenerator(t, options_).generate_struct_definition(p);
  }
  std::set<const OneofDescriptor *> oneof_set;
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    auto oneof = f->containing_oneof();
    if (oneof) {
      if (oneof_set.find(oneof) == oneof_set.end()) {
        OneofGenerator(oneof, options_).generate_definition(p);
        oneof_set.insert(oneof);
      }
      else {
        continue;
      }
    }
    else {
      fg_map_.get(f).generate_definition(p);
    }
  }
  if (options_.generate_eq_op) {
    format("bool operator==(const $1$&) const = default;\n", class_name(d_));
  }
  // format("std::size_t get_needed_size() const;\n");
  // format("std::string SerializeAsString() const;\n");
  // format("bool ParseFromArray(const char* data, std::size_t size);\n");
  format.outdent();
  format("};\n");
}
void MessageGenerator::generate_source(google::protobuf::io::Printer *p) {
  generate_get_needed_size(p);
  Formatter format(p);
  format("std::string $1$::SerializeAsString() const {\n", d_->name());
  format.indent();
  format("std::string buffer;\n");
  format("// calculate buffer size\n");
  format("std::size_t total = get_needed_size();\n");
  format("std::size_t pos = 0;\n");
  format("buffer.resize(total);\n");
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    //    FieldGenerator(f, options_).generate_calculate_size(p);
  }
  format("return buffer;\n");
  format.outdent();
  format("}\n");
}
void MessageGenerator::generate_get_needed_size(
    google::protobuf::io::Printer *p) {
  Formatter format(p);
  format("std::size_t total = unknown_fields.total_size();\n");
  std::set<const OneofDescriptor *> oneof_set;
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    fg_map_.get(f).generate_calculate_size(p, true);
  }
  format("return total;\n");
}
void MessageGenerator::generate_serialize_to(google::protobuf::io::Printer *p) {
  Formatter format(p);
  format("std::size_t pos = 0;\n");
  std::vector<const FieldDescriptor *> fs;
  fs.reserve(d_->field_count());
  for (int i = 0; i < d_->field_count(); ++i) {
    fs.push_back(d_->field(i));
  }
  std::sort(fs.begin(), fs.end(),
            [](const FieldDescriptor *lhs, const FieldDescriptor *rhs) {
              return lhs->number() < rhs->number();
            });
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = fs[i];
    format("{\n");
    format.indent();
    fg_map_.get(f).generate_serialization(p, true);
    format.outdent();
    format("}\n");
  }
  format("unknown_fields.serialize_to(data, pos, size);\n");
}
void MessageGenerator::generate_deserialize_to(
    google::protobuf::io::Printer *p) {
  Formatter format(p);
  format("std::size_t pos = 0;\n");
  format("bool ok = false;\n");
  format("while (pos < size) {\n");
  format.indent();
  p->Print(R"(uint64_t tag = 0;
ok = read_tag(data, pos, size, tag);
if (!ok) {
  return false;
}
switch(tag) {
)");
  format.indent();
  for (int i = 0; i < d_->field_count(); ++i) {
    auto f = d_->field(i);
    fg_map_.get(f).generate_deserialization(p);
  }
  format("default: {\n");
  format.indent();
  format("ok = deserialize_unknown(data, pos, size, tag, unknown_fields);\n");
  format("return ok;\n");
  format.outdent();
  format("}\n");
  format.outdent();
  format("}\n");

  format.outdent();
  format("}\n");
}
}  // namespace compiler
}  // namespace struct_pb