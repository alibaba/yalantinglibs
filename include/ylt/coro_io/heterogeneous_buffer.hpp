#pragma once
#include <string>
#include <variant>
#include <ylt/coro_io/memory_owner.hpp>

#include "ylt/coro_io/data_view.hpp"
namespace coro_io {
/**
 * @brief Heterogeneous buffer implementation supporting both CPU and GPU memory
 *
 * This buffer class supports both CPU memory (when gpu_id is -1) and GPU memory
 * (when gpu_id >= 0).
 */
class heterogeneous_buffer {
 private:
  std::variant<std::string, memory_owner_t>
      buffer_;  // Either CPU string or GPU memory

 public:
  /**
   * @brief Default constructor creates an empty CPU buffer
   */
  heterogeneous_buffer() = default;

  /**
   * @brief Constructor with initial size and GPU ID
   * @param size Initial buffer size
   * @param gpu_id GPU ID (-1 for CPU memory)
   */
  heterogeneous_buffer(std::size_t size, int gpu_id = -1) {
    if (gpu_id == -1) {
      buffer_.template emplace<std::string>(size, '\0');
    }
    else {
      buffer_.template emplace<memory_owner_t>(size, gpu_id);
    }
  }

  int gpu_id() const {
    if (buffer_.index() == 0) {
      return -1;
    }
    else {
      return std::get<memory_owner_t>(buffer_).gpu_id_;
    }
  }

  /**
   * @brief Move constructor
   */
  heterogeneous_buffer(heterogeneous_buffer&& other) noexcept = default;

  /**
   * @brief Move assignment operator
   */
  heterogeneous_buffer& operator=(heterogeneous_buffer&& other) noexcept =
      default;

  /**
   * @brief Get pointer to the underlying data
   * @return Pointer to buffer data
   */
  char* data() {
    return std::visit(
        [](auto& buffer) {
          return (char*)buffer.data();
        },
        buffer_);
  }

  /**
   * @brief Get const pointer to the underlying data
   * @return Const pointer to buffer data
   */
  const char* data() const {
    return std::visit(
        [](auto& buffer) {
          return (char*)buffer.data();
        },
        buffer_);
  }

  /**
   * @brief Get the size of the buffer
   * @return Size of the buffer in bytes
   */
  std::size_t size() const {
    return std::visit(
        [](auto& buffer) {
          return buffer.size();
        },
        buffer_);
  }

  /**
   * @brief Clear the buffer contents
   */
  void clear() { *this = heterogeneous_buffer(size(), gpu_id()); }

  /**
   * @brief Check if buffer is empty
   * @return True if buffer is empty, false otherwise
   */
  bool empty() const { return size() == 0; }

  /**
   * @brief Check if the buffer is using GPU memory
   * @return True if using GPU memory, false if using CPU memory
   */
  bool is_gpu_memory() const { return gpu_id() != -1; }

  /**
   * @brief Convert to boolean (non-empty check)
   */
  explicit operator bool() const { return !empty(); }

  operator std::string_view() const { return {data(), size()}; }

  operator coro_io::data_view() const {
    return {std::string_view{data(), size()}, gpu_id()};
  }

  std::string* get_string() noexcept {
    return std::get_if<std::string>(&buffer_);
  }
  memory_owner_t* get_gpu_buffer() noexcept {
    return std::get_if<memory_owner_t>(&buffer_);
  }
#ifdef YLT_ENABLE_CUDA
  operator std::string() { return *get_string(); }
#endif
};
}  // namespace coro_io