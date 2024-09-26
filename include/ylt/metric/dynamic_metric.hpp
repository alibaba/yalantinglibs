#pragma once
#include "metric.hpp"
namespace ylt::metric {

class dynamic_metric : public metric_t {
 public:
  using metric_t::metric_t;
};

template <typename core_type, uint8_t N>
class dynamic_metric_impl : public dynamic_metric {
  struct my_hash {
    using is_transparent = void;
    std::size_t operator()(
        const std::span<const std::string, N>& s) const noexcept {
      unsigned int seed = 131;
      unsigned int hash = 0;
      for (const auto& str : s) {
        for (auto ch : str) {
          hash = hash * seed + ch;
        }
      }
      return hash;
    }
    std::size_t operator()(
        const std::span<std::string_view, N>& s) const noexcept {
      unsigned int seed = 131;
      unsigned int hash = 0;
      for (const auto& str : s) {
        for (auto ch : str) {
          hash = hash * seed + ch;
        }
      }
      return hash;
    }
  };
  struct my_equal {
    bool operator()(const std::span<const std::string, N>& s1,
                    const std::span<const std::string, N>& s2) const noexcept {
      if constexpr (N > 0) {
        for (int i = 0; i < N; ++i) {
          if (s1[i] != s2[i]) {
            return false;
          }
        }
      }
      return true;
    }
  };
  using key_type = std::array<std::string, N>;
  struct metric_pair {
   public:
    key_type label;
    core_type value;
    template <typename T, typename... Args>
    metric_pair(T&& first, Args&&... args)
        : label(std::forward<T>(first)), value(std::forward<Args>(args)...) {
      g_user_metric_label_count->inc();
      if (ylt_label_max_age.count()) {
        tp = std::chrono::steady_clock::now();
      }
    }
    std::chrono::steady_clock::time_point get_created_time() const {
      return tp;
    }

   private:
    std::chrono::steady_clock::time_point tp;
  };

  struct value_type : public std::shared_ptr<metric_pair> {
    value_type() : std::shared_ptr<metric_pair>(nullptr) {}
    template <typename... Args>
    value_type(Args&&... args)
        : std::shared_ptr<metric_pair>(
              std::make_shared<metric_pair>(std::forward<Args>(args)...)){};
  };

 public:
  using dynamic_metric::dynamic_metric;
  size_t size() const {
    std::lock_guard guard(mtx_);
    return table.size();
  }
  size_t empty() const { return !size(); }
  size_t label_value_count() const { return size(); }

  std::vector<std::shared_ptr<metric_pair>> copy() const {
    std::lock_guard guard(mtx_);
    std::vector<std::shared_ptr<metric_pair>> ret;
    ret.reserve(table.size());
    for (auto& e : table) {
      ret.push_back(e.second);
    }
    return ret;
  }

 protected:
  template <typename Key, typename... Args>
  std::shared_ptr<metric_pair> try_emplace(Key&& key, Args&&... args) {
    std::lock_guard guard(mtx_);
    std::span<const std::string, N> view = key;
    auto iter = table.try_emplace(view, std::forward<Key>(key),
                                  std::forward<Args>(args)...);
    if (iter.second) {
      *const_cast<std::span<const std::string, N>*>(&iter.first->first) =
          iter.first->second->label;
    }
    return table
        .try_emplace(view, std::forward<Key>(key), std::forward<Args>(args)...)
        .first->second;
  }
  void clean_expired_label() override {
    erase_if([now = std::chrono::steady_clock::now()](auto& pair) mutable {
      bool r = std::chrono::duration_cast<std::chrono::seconds>(
                   now - pair.second->get_created_time())
                   .count() >= ylt_label_max_age.count();
      return r;
    });
  }
  std::shared_ptr<metric_pair> find(std::span<const std::string, N> key) {
    std::lock_guard guard(mtx_);
    auto iter = table.find(key);
    if (iter != table.end()) {
      return iter->second;
    }
    else {
      return nullptr;
    }
  }
  size_t erase(std::span<const std::string, N> key) {
    std::lock_guard guard(mtx_);
    return table.erase(key);
  }
  void erase_if(auto&& op) {
    std::lock_guard guard(mtx_);
    std::erase_if(table, op);
  }

 private:
  mutable std::mutex mtx_;
  std::unordered_map<std::span<const std::string, N>, value_type, my_hash,
                     my_equal>
      table;
};
}  // namespace ylt::metric