#pragma once
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

namespace ylt::metric {
inline size_t get_round_index(size_t size) {
  static std::atomic<size_t> round = 0;
  static thread_local size_t index = round++;
  return index % size;
}
template <typename value_type>
class thread_local_value {
 public:
  thread_local_value(size_t dupli_count = std::thread::hardware_concurrency())
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

  thread_local_value(thread_local_value &&other) {
    duplicates_ = std::move(other.duplicates_);
  }

  thread_local_value &operator=(thread_local_value &&other) {
    duplicates_ = std::move(other.duplicates_);
  }

  void inc(value_type value = 1) { local_value() += value; }

  void dec(value_type value = 1) { local_value() -= value; }

  value_type update(value_type value = 1) {
    value_type val = reset();
    local_value() = value;
    return val;
  }

  value_type reset() {
    value_type val = 0;
    for (auto &t : duplicates_) {
      if (t) {
        val += t.load()->exchange(0);
      }
    }
    return val;
  }

  auto &local_value() {
    static thread_local auto index = get_round_index(duplicates_.size());
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

 private:
  std::vector<std::atomic<std::atomic<value_type> *>> duplicates_;
};
}  // namespace ylt::metric
