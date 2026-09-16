#include "probe.hpp"

int main() {
  asio::io_context context;
  coro_io::ExecutorWrapper<> executor(context.get_executor());
  coro_io::random_coro_file selected(&executor);
  coro_io::native_random_coro_file native(&executor);
  coro_io::own_ring_random_coro_file own(&executor);
  return probe(selected) && !native.is_open() && !own.is_open() ? 0 : 1;
}
