#include <random>
namespace ylt::util {
template <typename engine_type = std::default_random_engine>
inline engine_type& random_engine() {
  static thread_local std::default_random_engine e(std::random_device{}());
  return e;
}
}  // namespace ylt::util