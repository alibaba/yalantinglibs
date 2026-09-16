#pragma once

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include "../own_ring_io_context.hpp"

namespace coro_io {

inline OwnRingIoContext &global_own_ring_context() {
  static auto *contexts = [] {
    unsigned count = 1;
    if (const char *value = std::getenv("YLT_OWN_RING_THREADS")) {
      std::string_view text(value);
      auto parsed =
          std::from_chars(text.data(), text.data() + text.size(), count);
      if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
          count == 0 || count > 64) {
        throw std::invalid_argument(
            "YLT_OWN_RING_THREADS must be between 1 and 64");
      }
    }
    auto pool =
        std::make_unique<std::vector<std::unique_ptr<OwnRingIoContext>>>();
    pool->reserve(count);
    for (unsigned index = 0; index < count; ++index) {
      auto context = std::make_unique<OwnRingIoContext>();
      if (!context->ok()) {
        throw std::system_error(-context->init_error(), std::system_category());
      }
      pool->push_back(std::move(context));
    }
    return pool.release();
  }();
  static std::atomic<size_t> next{0};
  return *(*contexts)[next.fetch_add(1, std::memory_order_relaxed) %
                      contexts->size()];
}

}  // namespace coro_io
