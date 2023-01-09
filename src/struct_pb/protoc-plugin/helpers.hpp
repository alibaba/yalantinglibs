#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

#include "Options.hpp"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
namespace struct_pb {
namespace compiler {
using namespace google::protobuf;
using namespace google::protobuf::compiler;

inline bool is_map_entry(const google::protobuf::Descriptor *descriptor) {
  return descriptor->options().map_entry();
}

void flatten_messages_in_file(
    const google::protobuf::FileDescriptor *file,
    std::vector<const google::protobuf::Descriptor *> *result);
inline std::vector<const google::protobuf::Descriptor *>
flatten_messages_in_file(const google::protobuf::FileDescriptor *file) {
  std::vector<const google::protobuf::Descriptor *> ret;
  flatten_messages_in_file(file, &ret);
  return ret;
}
template <typename F>
void for_each_message(const google::protobuf::Descriptor *descriptor,
                      F &&func) {
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    for_each_message(descriptor->nested_type(i), std::forward<F &&>(func));
  }
  func(descriptor);
}

template <typename F>
void for_each_message(const google::protobuf::FileDescriptor *descriptor,
                      F &&func) {
  for (int i = 0; i < descriptor->message_type_count(); ++i) {
    for_each_message(descriptor->message_type(i), std::forward<F &&>(func));
  }
}

inline void flatten_messages_in_file(
    const google::protobuf::FileDescriptor *file,
    std::vector<const google::protobuf::Descriptor *> *result) {
  for (int i = 0; i < file->message_type_count(); ++i) {
    for_each_message(file->message_type(i),
                     [&](const google::protobuf::Descriptor *descriptor) {
                       if (is_map_entry(descriptor)) {
                         return;
                       }
                       result->push_back(descriptor);
                     });
  }
}
inline std::string resolve_keyword(const std::string &name) {
  static std::set<std::string> keyword_set{
      //
      "NULL",
      "alignas",
      "alignof",
      "and",
      "and_eq",
      "asm",
      "auto",
      "bitand",
      "bitor",
      "bool",
      "break",
      "case",
      "catch",
      "char",
      "class",
      "compl",
      "const",
      "constexpr",
      "const_cast",
      "continue",
      "decltype",
      "default",
      "delete",
      "do",
      "double",
      "dynamic_cast",
      "else",
      "enum",
      "explicit",
      "export",
      "extern",
      "false",
      "float",
      "for",
      "friend",
      "goto",
      "if",
      "inline",
      "int",
      "long",
      "mutable",
      "namespace",
      "new",
      "noexcept",
      "not",
      "not_eq",
      "nullptr",
      "operator",
      "or",
      "or_eq",
      "private",
      "protected",
      "public",
      "register",
      "reinterpret_cast",
      "return",
      "short",
      "signed",
      "sizeof",
      "static",
      "static_assert",
      "static_cast",
      "struct",
      "switch",
      "template",
      "this",
      "thread_local",
      "throw",
      "true",
      "try",
      "typedef",
      "typeid",
      "typename",
      "union",
      "unsigned",
      "using",
      "virtual",
      "void",
      "volatile",
      "wchar_t",
      "while",
      "xor",
      "xor_eq",
      "char8_t",
      "char16_t",
      "char32_t",
      "concept",
      "consteval",
      "constinit",
      "co_await",
      "co_return",
      "co_yield",
      "requires",
  };
  auto it = keyword_set.find(name);
  if (it == keyword_set.end()) {
    return name;
  }
  return name + "_";
}
inline std::string class_name(const google::protobuf::Descriptor *descriptor) {
  //  assert(descriptor);
  auto parent = descriptor->containing_type();
  std::string ret;
  if (parent) {
    ret += class_name(parent) + "::";
  }
  ret += descriptor->name();
  return resolve_keyword(ret);
}
inline std::string enum_name(
    const google::protobuf::EnumDescriptor *descriptor) {
  auto parent = descriptor->containing_type();
  std::string ret;
  if (parent) {
    ret += class_name(parent) + "::";
  }
  ret += descriptor->name();
  return resolve_keyword(ret);
}
// https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
inline std::string ReplaceAll(std::string str, const std::string &from,
                              const std::string &to) {
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos +=
        to.length();  // Handles case where 'to' is a substring of 'from'
  }
  return str;
}
inline std::string dots_to_colons(std::string name) {
  return ReplaceAll(name, ".", "::");
}
inline std::string get_namespace(const std::string &package) {
  if (package.empty()) {
    return "";
  }
  return "::" + dots_to_colons(package);
}

inline std::string get_namespace(const google::protobuf::FileDescriptor *file,
                                 const Options &options) {
  if (file == options.f) {
    return get_namespace(options.ns);
  }
  else {
    return get_namespace(file->package());
  }
}

inline std::string qualified_file_level_symbol(
    const google::protobuf::FileDescriptor *file, const std::string &name,
    const Options &options) {
  //  if (options.ns.empty()) {
  //    return absl::StrCat("::", name);
  //  }
  return get_namespace(file, options) + "::" + name;
}

inline std::string qualified_class_name(const google::protobuf::Descriptor *d,
                                        const Options &options) {
  return qualified_file_level_symbol(d->file(), class_name(d), options);
}

inline std::string qualified_enum_name(
    const google::protobuf::EnumDescriptor *d, const Options &options) {
  return qualified_file_level_symbol(d->file(), enum_name(d), options);
}

class Formatter {
 public:
  Formatter(google::protobuf::io::Printer *printer) : printer_(printer) {}
  void indent() const { printer_->Indent(); }
  void outdent() const { printer_->Outdent(); }
  template <typename... Args>
  void operator()(const char *format, const Args &...args) const {
    printer_->FormatInternal({ToString(args)...}, vars_, format);
  }

 private:
  static std::string ToString(const std::string &s) { return s; }
  template <typename I, typename = typename std::enable_if<
                            std::is_integral<I>::value>::type>
  static std::string ToString(I x) {
    return std::to_string(x);
  }

 private:
  google::protobuf::io::Printer *printer_;
  std::map<std::string, std::string> vars_;
};

class NamespaceOpener {
 public:
  NamespaceOpener(google::protobuf::io::Printer *p,
                  const std::string &package = {});
  ~NamespaceOpener();

 public:
  void open() const;
  void close() const;

 private:
  std::vector<std::string> ns_;
  google::protobuf::io::Printer *p_;
};

}  // namespace compiler
}  // namespace struct_pb