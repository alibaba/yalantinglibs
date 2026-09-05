#pragma once

#include <fcntl.h>

#include <cerrno>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

namespace coro_io {

class weak_file_handle;

class shared_file_handle {
 public:
  using native_handle_type = int;

  shared_file_handle() noexcept = default;

  static std::pair<std::error_code, shared_file_handle> open(
      std::string_view path, int flags) noexcept {
    native_handle_type fd = -1;
    try {
      std::string null_terminated_path(path);
#if defined(_WIN32)
      fd = ::_open(null_terminated_path.c_str(), flags, _S_IREAD | _S_IWRITE);
#else
      fd = ::open(null_terminated_path.c_str(), flags,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
    } catch (...) {
      return {std::make_error_code(std::errc::not_enough_memory), {}};
    }
    if (fd < 0) {
      return {std::error_code(errno, std::generic_category()), {}};
    }

    auto handle = adopt(fd);
    if (!handle.valid()) {
      close_native_handle(fd);
      return {std::make_error_code(std::errc::not_enough_memory), {}};
    }
    return {{}, std::move(handle)};
  }

  static shared_file_handle adopt(native_handle_type fd) noexcept {
    if (fd < 0) {
      return {};
    }

    try {
      return shared_file_handle(std::make_shared<control_block>(fd));
    } catch (...) {
      return {};
    }
  }

  static std::pair<std::error_code, shared_file_handle> duplicate(
      native_handle_type fd) noexcept {
    if (fd < 0) {
      return {std::make_error_code(std::errc::bad_file_descriptor), {}};
    }

#if defined(_WIN32)
    native_handle_type duplicated_fd = ::_dup(fd);
#else
    native_handle_type duplicated_fd = ::dup(fd);
#endif
    if (duplicated_fd < 0) {
      return {std::error_code(errno, std::generic_category()), {}};
    }

    auto handle = adopt(duplicated_fd);
    if (!handle.valid()) {
      close_native_handle(duplicated_fd);
      return {std::make_error_code(std::errc::not_enough_memory), {}};
    }
    return {{}, std::move(handle)};
  }

  bool valid() const noexcept { return state_ && state_->fd >= 0; }

  native_handle_type native_handle() const noexcept {
    return valid() ? state_->fd : -1;
  }

  weak_file_handle weak() const noexcept;

 private:
  static void close_native_handle(native_handle_type fd) noexcept {
#if defined(_WIN32)
    ::_close(fd);
#else
    ::close(fd);
#endif
  }

  struct control_block {
    explicit control_block(native_handle_type value) noexcept : fd(value) {}

    ~control_block() noexcept {
      if (fd >= 0) {
        close_native_handle(fd);
      }
    }

    native_handle_type fd = -1;
  };

  explicit shared_file_handle(std::shared_ptr<control_block> state) noexcept
      : state_(std::move(state)) {}

  std::shared_ptr<control_block> state_;
  friend class weak_file_handle;
};

class weak_file_handle {
 public:
  weak_file_handle() noexcept = default;

  shared_file_handle lock() const noexcept {
    return shared_file_handle(state_.lock());
  }

  bool expired() const noexcept { return state_.expired(); }

 private:
  explicit weak_file_handle(
      std::weak_ptr<shared_file_handle::control_block> state) noexcept
      : state_(std::move(state)) {}

  std::weak_ptr<shared_file_handle::control_block> state_;
  friend class shared_file_handle;
};

inline weak_file_handle shared_file_handle::weak() const noexcept {
  return weak_file_handle(state_);
}

}  // namespace coro_io
