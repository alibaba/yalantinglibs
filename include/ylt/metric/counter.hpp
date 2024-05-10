#pragma once
#include "metric.hpp"

namespace ylt {
class counter_t : public metric_t {
 public:
  counter_t() = default;
  counter_t(std::string name, std::string help,
            std::vector<std::string> labels_name = {})
      : metric_t(MetricType::Counter, std::move(name), std::move(help),
                 std::move(labels_name)) {}

  counter_t(const char *name, const char *help,
            std::vector<const char *> labels_name = {})
      : counter_t(
            std::string(name), std::string(help),
            std::vector<std::string>(labels_name.begin(), labels_name.end())) {}

  void inc() {
    std::lock_guard guard(mtx_);
    set_value(value_map_[{}], 1, op_type_t::INC);
  }

  void inc(const std::vector<std::string> &labels_value, double value = 1) {
    if (value == 0) {
      return;
    }
    validate(labels_value, value);
    std::lock_guard guard(mtx_);
    set_value(value_map_[labels_value], value, op_type_t::INC);
  }

  void update(const std::vector<std::string> &labels_value, double value) {
    if (labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
    std::lock_guard guard(mtx_);
    set_value(value_map_[labels_value], value, op_type_t::SET);
  }

  void reset() {
    std::lock_guard guard(mtx_);
    for (auto &pair : value_map_) {
      pair.second = {};
    }
  }

  std::map<std::vector<std::string>, sample_t,
           std::less<std::vector<std::string>>>
  values(bool need_lock = true) override {
    if (need_lock) {
      return value_map_;
    }
    std::lock_guard guard(mtx_);
    return value_map_;
  }

  void serialize(std::string &str) override {
    if (value_map_.empty()) {
      return;
    }
    str.append("# HELP ").append(name_).append(" ").append(help_).append("\n");
    str.append("# TYPE ")
        .append(name_)
        .append(" ")
        .append(metric_name())
        .append("\n");
    for (auto &[labels_value, sample] : value_map_) {
      str.append(name_);
      if (labels_name_.empty()) {
        str.append(" ");
      }
      else {
        str.append("{");
        build_string(str, labels_name_, labels_value);
        str.append("} ");
      }

      str.append(std::to_string((int64_t)sample.value));
      if (enable_timestamp_) {
        str.append(" ");
        str.append(std::to_string(sample.timestamp));
      }
      str.append("\n");
    }
  }

 protected:
  enum class op_type_t { INC, DEC, SET };
  void build_string(std::string &str, const std::vector<std::string> &v1,
                    const std::vector<std::string> &v2) {
    for (size_t i = 0; i < v1.size(); i++) {
      str.append(v1[i]).append("=\"").append(v2[i]).append("\"").append(",");
    }
    str.pop_back();
  }

  void validate(const std::vector<std::string> &labels_value, double value) {
    if (value < 0) {
      throw std::invalid_argument("the value is less than zero");
    }
    if (labels_name_.size() != labels_value.size()) {
      throw std::invalid_argument(
          "the number of labels_value name and labels_value is not match");
    }
  }

  void set_value(sample_t &sample, double value, op_type_t type) {
    sample.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    switch (type) {
      case op_type_t::INC:
        sample.value += value;
        break;
      case op_type_t::DEC:
        sample.value -= value;
        break;
      case op_type_t::SET:
        sample.value = value;
        break;
    }
  }

  std::mutex mtx_;
  std::map<std::vector<std::string>, sample_t,
           std::less<std::vector<std::string>>>
      value_map_;
};
}  // namespace ylt