#pragma once
#include <string>
// compat only
namespace absl {
template <typename T>
auto to(const T& t) {
  if constexpr (std::same_as<T, std::string>) {
    return t.c_str();
  }
  else {
    return t;
  }
}
class SimpleAppender {
 public:
  SimpleAppender(std::string& buffer, std::string_view format)
      : buffer(buffer), format_(format) {}
  void format() {
    buffer += format_.substr(index);
    index = format_.size();
  }
  template <typename T, typename... Args>
  void format(const T& t, const Args&... args) {
    while (index < format_.size()) {
      if (format_[index] != '%') {
        buffer += format_[index];
        index++;
        continue;
      }
      if (index + 1 < format_.size()) {
        if (format_[index + 1] == 's') {
          if constexpr (std::same_as<std::string, T>) {
            buffer += t;
          }
          else if constexpr (std::same_as<std::string_view, T>) {
            buffer += t;
          }
          else if constexpr (std::same_as<char*, std::decay_t<T>>) {
            buffer += std::string(t);
          }
          else {
            buffer += "'ERROR FORMAT'";
          }
          index++;
          index++;
          format(args...);
        }
        else if (format_[index + 1] == 'd') {
          if constexpr (std::integral<T>) {
            buffer += std::to_string(t);
          }
          else {
            buffer += "'ERROR FORMAT'";
          }
          index++;
          index++;
          format(args...);
        }
        else if (format_[index + 1] == 'z') {
          if (index + 2 < format_.size() && format_[index + 2] == 'u') {
            if constexpr (std::integral<T>) {
              buffer += std::to_string(t);
            }
            else {
              buffer += "'ERROR FORMAT'";
            }
            index++;
            index++;
            index++;
            format(args...);
          }
        }
      }
      else {
        buffer += "'maybe ERROR FORMAT'";
        break;
      }
    }
  }

 private:
  std::string& buffer;
  std::string_view format_;
  int index = 0;
};
template <typename... Args>
void StrAppendFormat(std::string* output, const char* format,
                     const Args&... args) {
  SimpleAppender(*output, format).format(args...);
  //  std::cout << "result: " << *output << std::endl;
}
}  // namespace absl