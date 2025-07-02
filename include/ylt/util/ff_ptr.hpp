#pragma once
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>
#include <iostream>
#if __has_include(<stacktrace>) && __cplusplus >= 202302L
#include <stacktrace>
#endif
#include "b_stacktrace.h"
namespace ylt::util {
  
enum class ff_ptr_error_type {
  heap_use_after_free = 0,
  double_free = 1,
  use_after_scope = 2,
};

inline std::string to_string(ff_ptr_error_type error) {
  switch (error) {
    case ff_ptr_error_type::heap_use_after_free:
      return "heap use after free";
    case ff_ptr_error_type::double_free:
      return "double free";
    case ff_ptr_error_type::use_after_scope:
      return "use after scope";
    default:
      return "unknown error";
  }
}
namespace detail {

constexpr inline uint64_t magic_poison_value = 0xECFD'A8B9'6475'2031;
inline bool is_poisoned(void* ptr) {
  if (ptr) [[likely]]
    return memcmp((char*)ptr, (char*)&magic_poison_value, sizeof(uint64_t))==0;
  else
    return false;
}
template<typename T>
inline void poisoned(T* ptr) {
  if (ptr) [[likely]] {
    if constexpr (sizeof(T)<12) {
      memcpy((char*)ptr, (char*)&magic_poison_value, sizeof(uint64_t));
    }
    else {
      memcpy(((char*)ptr), (char*)&magic_poison_value, sizeof(uint64_t));
    }
  }
}

inline static auto default_error_handler = [](ff_ptr_error_type error, void* raw_pointer) {
        std::cerr
            << "==========================================================\n"
               "ff_ptr: "
            << raw_pointer << "check failed:" << to_string(error) << "\n"
            << std::endl;
#if __cpp_lib_stacktrace && __cplusplus >= 202302L
        std::cerr << std::stacktrace::current() << std::endl;
#else
        auto str = b_stacktrace_get_string();
        std::cerr << str << std::endl;
        free(str);
#endif
      };
inline std::shared_ptr<std::function<void(ff_ptr_error_type, void*)>> get_error_handler(std::function<void(ff_ptr_error_type, void*)> handler= default_error_handler) {
  static auto error_handler = std::make_shared<std::function<void(ff_ptr_error_type, void*)>>(std::move(handler));
  return error_handler;
}

template<typename T>
inline void* get_pointer_offset(T* raw_pointer) noexcept {
  if (sizeof(T) >= 12) {
    return ((char*)raw_pointer);
  }
  return raw_pointer;
}

template<typename T>
inline bool ptr_check(ff_ptr_error_type error, T* raw_pointer) noexcept {
  if (is_poisoned(get_pointer_offset(raw_pointer))) [[unlikely]] {
    (*get_error_handler())(error, raw_pointer);
    return false;
  }
  else {
    return true;
  }
}
}  // namespace detail

inline void regist_ff_ptr_error_handler(std::function<void(ff_ptr_error_type, void*)> func) {
  detail::get_error_handler(std::move(func));
}

template <typename T>
class ff_ptr {
  T* raw_pointer = nullptr;
public:
  ff_ptr() noexcept = default;
  ff_ptr(const ff_ptr&) noexcept = default;
  ff_ptr(T* raw_pointer) : raw_pointer(raw_pointer) {}
  ff_ptr* operator->() {
    if constexpr (sizeof(T) >= sizeof(uint64_t)) {
      detail::ptr_check(ff_ptr_error_type::heap_use_after_free, raw_pointer);
    }
    return raw_pointer;
  }
  operator T*() const noexcept {
    if constexpr (sizeof(T) >= sizeof(uint64_t)) {
      detail::ptr_check(ff_ptr_error_type::heap_use_after_free, raw_pointer);
    }
    return raw_pointer;
  }
};

template <typename Derived>
struct ff_Base {
  ff_Base() {
    if constexpr (sizeof(Derived) >= 8) {
      poisoned(this);
    }
  }
  ~ff_Base() {
    if constexpr (sizeof(Derived) >= 8) {
      uint64_t* data = (uint64_t*)this;
      if (detail::ptr_check(ff_ptr_error_type::use_after_scope, data)) [[unlikely]] {
        detail::poisoned(this);
      }
    }
  }
};


template <typename T>
void ff_delete(ff_ptr<T> p) {
  if constexpr (!std::derived_from<T, ff_Base<T>> && sizeof(T) >= sizeof(uint64_t)) { 
    uint64_t* data = (uint64_t*)p.get();
    detail::ptr_check(ff_ptr_error_type::double_free, data);
    if (p) {
      p->~T();
      poisoned(p);
      std::free(p);
    }
  }
  else {
    delete p;
  }
}


template <typename T, typename... Args>
ff_ptr<T> ff_new(Args&&... args) {
  if constexpr (std::derived_from<T, ff_Base<T>>) {
    return new T{std::forward<Args>(args)...};
  }
  else if constexpr (sizeof(T) >= sizeof(uint64_t)) { 
    T * buf = std::malloc(sizeof(T));
    *(uint64_t*)buf = 0; //erase magic number
    return new (buf) T{std::forward<Args>(args)...};
  }
  else {
    return new T{std::forward<Args>(args)...};
  }
}


template<typename T>
struct ff_shared_ptr: public std::shared_ptr<T> {
  using std::shared_ptr<T>::shared_ptr;
  ff_shared_ptr(const std::shared_ptr<T>& p): std::shared_ptr<T>(p){}
  ff_shared_ptr(std::shared_ptr<T>&& p): std::shared_ptr<T>(std::move(p)){}
  ff_shared_ptr(const ff_shared_ptr &p)=default;
  ff_shared_ptr(ff_shared_ptr &&p)=default;
  ff_shared_ptr& operator=(const ff_shared_ptr &p)=default;
  ff_shared_ptr& operator=(ff_shared_ptr &&p)=default;
  operator std::shared_ptr<T>() & {
    return std::shared_ptr<T>(*this);
  }
  operator std::shared_ptr<T>() &&{
    return std::shared_ptr<T>(std::move(*this));
  }
  ~ff_shared_ptr() {
    if constexpr (!std::derived_from<T, ff_Base<T>> && sizeof(T) >= sizeof(uint64_t)) { 
      char* data = reinterpret_cast<char*>(this->get());
      if (detail::ptr_check(ff_ptr_error_type::double_free, data)) [[unlikely]] {
        detail::poisoned(this->get());
      }
    }
  }
};



template<typename T, typename Deleter = void>
struct ff_unique_ptr: public std::unique_ptr<T,Deleter> {
  using std::unique_ptr<T,Deleter>::unique_ptr;
  ff_unique_ptr(std::unique_ptr<T,Deleter>&& p): std::unique_ptr<T,Deleter>(std::move(p)){}
  ff_unique_ptr(const ff_unique_ptr &p)=default;
  ff_unique_ptr(ff_unique_ptr &&p)=default;
  ff_unique_ptr& operator=(const ff_unique_ptr &p)=default;
  ff_unique_ptr& operator=(ff_unique_ptr &&p)=default;

  operator std::unique_ptr<T,Deleter>() &&{
    return std::move(*this);
  }
  ~ff_unique_ptr() {
    if constexpr (!std::derived_from<T, ff_Base<T>> && sizeof(T) >= sizeof(uint64_t)) { 
      char* data = reinterpret_cast<char*>(this->get());
      if (detail::ptr_check(ff_ptr_error_type::double_free, data)) [[unlikely]] {
        detail::poisoned(this->get());
      }
    }
  }
};

template<typename T>

struct ff_unique_ptr<T,void>: public std::unique_ptr<T> {
  using std::unique_ptr<T>::unique_ptr;

  ff_unique_ptr(std::unique_ptr<T>&& p): std::unique_ptr<T>(std::move(p)){}
  ff_unique_ptr(const ff_unique_ptr &p)=default;
  ff_unique_ptr(ff_unique_ptr &&p)=default;
  ff_unique_ptr& operator=(const ff_unique_ptr &p)=default;
  ff_unique_ptr& operator=(ff_unique_ptr &&p)=default;

  operator std::unique_ptr<T>() &&{
    return std::move(*this);
  }
  ~ff_unique_ptr() {
    if constexpr (!std::derived_from<T, ff_Base<T>> && sizeof(T) >= sizeof(uint64_t)) { 
      char* data = reinterpret_cast<char*>(this->get());
      if (detail::ptr_check(ff_ptr_error_type::double_free, data)) [[unlikely]] {
        detail::poisoned(this->get());
      }
    }
  }
};


template <typename T, typename... Args>
ff_unique_ptr<T> ff_make_unique(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
ff_shared_ptr<T> ff_make_shared(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}  // namespace ylt::util