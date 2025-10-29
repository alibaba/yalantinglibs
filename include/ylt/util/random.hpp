#include <random>
namespace ylt::util {
  auto& random_engine() {
    static thread_local std::default_random_engine e(std::random_device{}());
    return e;
  }
}