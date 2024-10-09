#pragma once
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace ylt::util {
namespace internal {
template <typename Map>
class map_lock_t {
 public:
  using key_type = typename Map::key_type;
  using value_type = typename Map::mapped_type;
  using const_iterator = typename Map::const_iterator;

  size_t size() const {
    std::lock_guard lock(mtx_);
    return map_.size();
  }

  std::pair<const_iterator, bool> find(const key_type& key) {
    std::lock_guard lock(mtx_);
    auto it = map_.find(key);
    bool r = (it != map_.end());
    return std::make_pair(it, r);
  }

  template <typename... Args>
  auto try_emplace(const key_type& key, Args&&... args) {
    std::lock_guard lock(mtx_);
    return map_.try_emplace(key, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto try_emplace(key_type&& key, Args&&... args) {
    std::lock_guard lock(mtx_);
    return map_.try_emplace(std::move(key), std::forward<Args>(args)...);
  }

  size_t erase(const key_type& key) {
    std::lock_guard lock(mtx_);
    return map_.erase(key);
  }

  template <typename Func>
  void erase_if(Func&& op) {
    std::lock_guard guard(mtx_);
    std::erase_if(map_, std::forward<Func>(op));
  }

  template <typename T>
  auto copy() const {
    std::vector<T> vec;
    std::lock_guard guard(mtx_);
    for (auto& e : map_) {
      vec.push_back(e.second);
    }
    return vec;
  }

 private:
  mutable std::mutex mtx_;
  Map map_;
};
}  // namespace internal

template <typename Map, typename Hash>
class map_sharded_t {
 public:
  using key_type = typename Map::key_type;
  using value_type = typename Map::mapped_type;
  map_sharded_t(size_t shard_num, Hash func)
      : shards_(shard_num), hash_func_(std::move(func)) {}

  template <typename... Args>
  auto try_emplace(key_type&& key, Args&&... args) {
    return get_sharded(hash_func_(key))
        .try_emplace(std::move(key), std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto try_emplace(const key_type& key, Args&&... args) {
    return get_sharded(hash_func_(key))
        .try_emplace(key, std::forward<Args>(args)...);
  }

  size_t size() const {
    size_t size = 0;
    for (auto& map : shards_) {
      size += map.size();
    }
    return size;
  }

  auto find(const key_type& key) {
    return get_sharded(hash_func_(key)).find(key);
  }

  size_t erase(const key_type& key) {
    return get_sharded(hash_func_(key)).erase(key);
  }

  template <typename Func>
  void erase_if(Func&& op) {
    for (auto& map : shards_) {
      map.erase_if(std::forward<Func>(op));
    }
  }

  template <typename T>
  std::vector<T> copy() const {
    std::vector<T> ret;
    for (auto& map : shards_) {
      auto vec = map.template copy<T>();
      if (!vec.empty()) {
        ret.insert(ret.end(), vec.begin(), vec.end());
      }
    }
    return ret;
  }

 private:
  internal::map_lock_t<Map>& get_sharded(size_t hash) {
    return shards_[hash % shards_.size()];
  }

  std::vector<internal::map_lock_t<Map>> shards_;
  Hash hash_func_;
};
}  // namespace ylt::util