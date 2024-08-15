#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

namespace ylt::metric {
inline uint32_t get_round_index(uint32_t size) {
  static std::atomic<uint32_t> round = 0;
  static thread_local uint32_t index = round++;
  return index % size;
}
template <typename value_type>
class thread_local_value {
 public:
  thread_local_value(uint32_t dupli_count = std::thread::hardware_concurrency())
      : duplicates_(dupli_count) {}

  ~thread_local_value() {
    for (auto &t : duplicates_) {
      if (t) {
        delete t.load();
      }
    }
  }

  thread_local_value(const thread_local_value &other)
      : duplicates_(other.duplicates_.size()) {
    for (size_t i = 0; i < other.duplicates_.size(); i++) {
      if (other.duplicates_[i]) {
        auto ptr =
            new std::atomic<value_type>(other.duplicates_[i].load()->load());
        duplicates_[i] = ptr;
      }
    }
  }

  thread_local_value &operator=(const thread_local_value &other) {
    for (size_t i = 0; i < other.duplicates_.size(); i++) {
      if (other.duplicates_[i]) {
        auto ptr =
            new std::atomic<value_type>(other.duplicates_[i].load()->load());
        duplicates_[i] = ptr;
      }
    }
    return *this;
  }

  thread_local_value(thread_local_value &&other) {
    duplicates_ = std::move(other.duplicates_);
  }

  thread_local_value &operator=(thread_local_value &&other) {
    duplicates_ = std::move(other.duplicates_);
    return *this;
  }

  void inc(value_type value = 1) { local_value() += value; }

  void dec(value_type value = 1) { local_value() -= value; }

  value_type update(value_type value = 1) {
    value_type val = get_value(0).exchange(value);
    for (size_t i = 1; i < duplicates_.size(); i++) {
      if (duplicates_[i]) {
        val += duplicates_[i].load()->exchange(0);
      }
    }
    return val;
  }

  value_type reset() { return update(0); }

  auto &local_value() {
    auto index = get_round_index(duplicates_.size());
    return get_value(index);
  }

  auto &get_value(size_t index) {
    if (duplicates_[index] == nullptr) {
      auto ptr = new std::atomic<value_type>(0);

      std::atomic<value_type> *expected = nullptr;
      if (!duplicates_[index].compare_exchange_strong(expected, ptr)) {
        delete ptr;
      }
    }
    return *duplicates_[index];
  }

  value_type value() {
    value_type val = 0;
    for (auto &t : duplicates_) {
      if (t) {
        val += t.load()->load();
      }
    }
    return val;
  }

  void set_created_time(std::chrono::system_clock::time_point tm) {
    created_time_ = tm;
  }

  auto get_created_time() { return created_time_; }

 private:
  std::vector<std::atomic<std::atomic<value_type> *>> duplicates_;
  std::chrono::system_clock::time_point created_time_{};
};
}  // namespace ylt::metric
