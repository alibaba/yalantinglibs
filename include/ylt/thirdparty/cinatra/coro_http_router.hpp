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
#include "cinatra/coro_radix_tree.hpp"
#include "cinatra/response_cv.hpp"
#include "cinatra/utils.hpp"
#include "coro_http_response.hpp"
#include "ylt/util/type_traits.h"

namespace cinatra {
template <template <typename...> class U, typename T>
struct is_template_instant_of : std::false_type {};

template <template <typename...> class U, typename... args>
struct is_template_instant_of<U, U<args...>> : std::true_type {};

template <typename T>
constexpr inline bool is_lazy_v =
    is_template_instant_of<async_simple::coro::Lazy,
                           std::remove_cvref_t<T>>::value;

struct base_aspect {
  virtual bool before(coro_http_request& req, coro_http_response& resp) {
    return true;
  }

  virtual bool after(coro_http_request& req, coro_http_response& resp) {
    return true;
  }
};

class coro_http_router {
 public:
  // eg: "GET hello/" as a key
  template <http_method method, typename Func>
  void set_http_handler(
      std::string key, Func handler,
      std::vector<std::shared_ptr<base_aspect>> aspects = {}) {
    constexpr auto method_name = cinatra::method_name(method);
    std::string whole_str;
    whole_str.append(method_name).append(" ").append(key);

    // hold keys to make sure map_handles_ key is
    // std::string_view, avoid memcpy when route
    using return_type = typename util::function_traits<Func>::return_type;
    if constexpr (is_lazy_v<return_type>) {
      if (whole_str.find(":") != std::string::npos) {
        std::vector<std::string> coro_method_names = {};
        std::string coro_method_str;
        coro_method_str.append(method_name);
        coro_method_names.push_back(coro_method_str);
        coro_router_tree_->coro_insert(key, std::move(handler),
                                       coro_method_names);
      }
      else {
        if (whole_str.find("{") != std::string::npos ||
            whole_str.find(")") != std::string::npos) {
          std::string pattern = whole_str;

          if (pattern.find("{}") != std::string::npos) {
            replace_all(pattern, "{}", "([^/]+)");
          }

          coro_regex_handles_.emplace_back(std::regex(pattern),
                                           std::move(handler));
        }
        else {
          auto [it, ok] = coro_keys_.emplace(std::move(whole_str));
          if (!ok) {
            CINATRA_LOG_WARNING << key << " has already registered.";
            return;
          }
          coro_handles_.emplace(*it, std::move(handler));
          if (!aspects.empty()) {
            has_aspects_ = true;
            aspects_.emplace(*it, std::move(aspects));
          }
        }
      }
    }
    else {
      if (whole_str.find(':') != std::string::npos) {
        std::vector<std::string> method_names = {};
        std::string method_str;
        method_str.append(method_name);
        method_names.push_back(method_str);
        router_tree_->insert(whole_str, std::move(handler), method_names);
      }
      else if (whole_str.find("{") != std::string::npos ||
               whole_str.find(")") != std::string::npos) {
        std::string pattern = whole_str;

        if (pattern.find("{}") != std::string::npos) {
          replace_all(pattern, "{}", "([^/]+)");
        }

        regex_handles_.emplace_back(std::regex(pattern), std::move(handler));
      }
      else {
        auto [it, ok] = keys_.emplace(std::move(whole_str));
        if (!ok) {
          CINATRA_LOG_WARNING << key << " has already registered.";
          return;
        }
        map_handles_.emplace(*it, std::move(handler));
        if (!aspects.empty()) {
          has_aspects_ = true;
          aspects_.emplace(*it, std::move(aspects));
        }
      }
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

  void route(auto handler, auto& req, auto& resp, std::string_view key) {
    try {
      if (has_aspects_) {
        auto [it, ok] = handle_aspects(req, resp, key, true);
        if (!ok) {
          return;
        }
        (*handler)(req, resp);
        if (it != aspects_.end()) {
          handle_aspects(req, resp, it->second, false);
        }
      }
      else {
        (*handler)(req, resp);
      }
    } catch (const std::exception& e) {
      CINATRA_LOG_WARNING << "exception in business function, reason: "
                          << e.what();
      resp.set_status(status_type::service_unavailable);
    } catch (...) {
      CINATRA_LOG_WARNING << "unknown exception in business function";
      resp.set_status(status_type::service_unavailable);
    }
  }

  async_simple::coro::Lazy<void> route_coro(auto handler, auto& req, auto& resp,
                                            std::string_view key) {
    try {
      if (has_aspects_) {
        auto [it, ok] = handle_aspects(req, resp, key, true);
        if (!ok) {
          co_return;
        }
        co_await (*handler)(req, resp);
        if (it != aspects_.end()) {
          handle_aspects(req, resp, it->second, false);
        }
      }
      else {
        co_await (*handler)(req, resp);
      }
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

  std::shared_ptr<radix_tree> get_router_tree() { return router_tree_; }

  std::shared_ptr<radix_tree> get_coro_router_tree() {
    return coro_router_tree_;
  }

  const auto& get_coro_regex_handlers() { return coro_regex_handles_; }

  const auto& get_regex_handlers() { return regex_handles_; }

  bool handle_aspects(auto& req, auto& resp, auto& aspects, bool before) {
    bool r = true;
    for (auto& aspect : aspects) {
      if (before) {
        r = aspect->before(req, resp);
      }
      else {
        r = aspect->after(req, resp);
      }
      if (!r) {
        break;
      }
    }
    return r;
  }

  auto handle_aspects(auto& req, auto& resp, std::string_view key,
                      bool before) {
    decltype(aspects_.begin()) it;
    if (it = aspects_.find(key); it != aspects_.end()) {
      auto& aspects = it->second;
      bool r = handle_aspects(req, resp, aspects, before);
      if (!r) {
        return std::make_pair(aspects_.end(), false);
      }
    }

    return std::make_pair(it, true);
  }

  void handle_after() {}

 private:
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

  std::shared_ptr<radix_tree> router_tree_ =
      std::make_shared<radix_tree>(radix_tree());

  std::shared_ptr<radix_tree> coro_router_tree_ =
      std::make_shared<radix_tree>(radix_tree());

  std::vector<std::tuple<
      std::regex,
      std::function<void(coro_http_request& req, coro_http_response& resp)>>>
      regex_handles_;

  std::vector<std::tuple<
      std::regex, std::function<async_simple::coro::Lazy<void>(
                      coro_http_request& req, coro_http_response& resp)>>>
      coro_regex_handles_;

  std::unordered_map<std::string_view,
                     std::vector<std::shared_ptr<base_aspect>>>
      aspects_;
  bool has_aspects_ = false;
};
}  // namespace cinatra