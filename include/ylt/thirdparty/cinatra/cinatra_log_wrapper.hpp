#include <iostream>
namespace cinatra {
struct null_logger_t {
  template <typename T>
  const null_logger_t& operator<<(T&&) const {
    return *this;
  }
};
struct cout_logger_t {
  template <typename T>
  const cout_logger_t& operator<<(T&& t) const {
    std::cout << std::forward<T>(t);
    return *this;
  }
  ~cout_logger_t() { std::cout << std::endl; }
};
struct cerr_logger_t {
  template <typename T>
  const cerr_logger_t& operator<<(T&& t) const {
    std::cerr << std::forward<T>(t);
    return *this;
  }
  ~cerr_logger_t() { std::cerr << std::endl; }
};

constexpr inline cinatra::null_logger_t NULL_LOGGER;

}  // namespace cinatra

#ifdef CINATRA_LOG_ERROR
#else
#define CINATRA_LOG_ERROR \
  cerr_logger_t {}
#endif

#ifdef CINATRA_LOG_WARNING
#else
#ifndef NDEBUG
#define CINATRA_LOG_WARNING \
  cerr_logger_t {}
#else
#define CINATRA_LOG_WARNING NULL_LOGGER
#endif
#endif

#ifdef CINATRA_LOG_INFO
#else
#ifndef NDEBUG
#define CINATRA_LOG_INFO \
  cout_logger_t {}
#else
#define CINATRA_LOG_INFO NULL_LOGGER
#endif
#endif

#ifdef CINATRA_LOG_DEBUG
#else
#ifndef NDEBUG
#define CINATRA_LOG_DEBUG \
  cout_logger_t {}
#else
#define CINATRA_LOG_DEBUG NULL_LOGGER
#endif
#endif

#ifdef CINATRA_LOG_TRACE
#else
#ifndef NDEBUG
#define CINATRA_LOG_TRACE \
  cout_logger_t {}
#else
#define CINATRA_LOG_TRACE NULL_LOGGER
#endif
#endif
