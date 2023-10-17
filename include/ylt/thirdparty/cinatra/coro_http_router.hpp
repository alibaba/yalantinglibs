#pragma once
#include <async_simple/coro/Lazy.h>

#include <algorithm>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

#include "cinatra/cinatra_log_wrapper.hpp"
#include "cinatra/coro_http_request.hpp"
#include "cinatra/function_traits.hpp"
#include "cinatra/response_cv.hpp"
#include "coro_http_response.hpp"

namespace cinatra {
template <template <typename...> class U, typename T>
struct is_template_instant_of : std::false_type {};

template <template <typename...> class U, typename... args>
struct is_template_instant_of<U, U<args...>> : std::true_type {};

template <typename T>
constexpr inline bool is_lazy_v =
    is_template_instant_of<async_simple::coro::Lazy,
                           std::remove_cvref_t<T>>::value;

class coro_http_router {
 public:
  static coro_http_router& instance() {
    static coro_http_router instance;
    return instance;
  }

  // eg: "GET hello/" as a key
  template <http_method method, typename Func>
  void set_http_handler(std::string key, Func handler) {
    constexpr auto method_name = cinatra::method_name(method);
    std::string whole_str;
    whole_str.append(method_name).append(" ").append(key);

    // hold keys to make sure map_handles_ key is
    // std::string_view, avoid memcpy when route
    using return_type = typename timax::function_traits<Func>::result_type;
    if constexpr (is_lazy_v<return_type>) {
      auto [it, ok] = coro_keys_.emplace(std::move(whole_str));
      if (!ok) {
        CINATRA_LOG_WARNING << key << " has already registered.";
        return;
      }
      coro_handles_.emplace(*it, std::move(handler));
    }
    else {
      auto [it, ok] = keys_.emplace(std::move(whole_str));
      if (!ok) {
        CINATRA_LOG_WARNING << key << " has already registered.";
        return;
      }
      map_handles_.emplace(*it, std::move(handler));
    }
  }

  std::function<void(coro_http_request& req, coro_http_response& resp)>*
  get_handler(std::string_view key) {
    if (auto it = map_handles_.find(key); it != map_handles_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  std::function<async_simple::coro::Lazy<void>(coro_http_request& req,
                                               coro_http_response& resp)>*
  get_coro_handler(std::string_view key) {
    if (auto it = coro_handles_.find(key); it != coro_handles_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  void route(auto handler, auto& req, auto& resp) {
    try {
      (*handler)(req, resp);
    } catch (const std::exception& e) {
      CINATRA_LOG_WARNING << "exception in business function, reason: "
                          << e.what();
      resp.set_status(status_type::service_unavailable);
    } catch (...) {
      CINATRA_LOG_WARNING << "unknown exception in business function";
      resp.set_status(status_type::service_unavailable);
    }
  }

  async_simple::coro::Lazy<void> route_coro(auto handler, auto& req,
                                            auto& resp) {
    try {
      co_await (*handler)(req, resp);
    } catch (const std::exception& e) {
      CINATRA_LOG_WARNING << "exception in business function, reason: "
                          << e.what();
      resp.set_status(status_type::service_unavailable);
    } catch (...) {
      CINATRA_LOG_WARNING << "unknown exception in business function";
      resp.set_status(status_type::service_unavailable);
    }
  }

  const auto& get_handlers() const { return map_handles_; }

  const auto& get_coro_handlers() const { return coro_handles_; }

 private:
  coro_http_router() = default;
  std::set<std::string> keys_;
  std::unordered_map<
      std::string_view,
      std::function<void(coro_http_request& req, coro_http_response& resp)>>
      map_handles_;

  std::set<std::string> coro_keys_;
  std::unordered_map<std::string_view,
                     std::function<async_simple::coro::Lazy<void>(
                         coro_http_request& req, coro_http_response& resp)>>
      coro_handles_;
};
}  // namespace cinatra