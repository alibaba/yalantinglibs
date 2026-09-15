#include "probe.hpp"

bool probe(coro_io::random_coro_file &file) {
  coro_io::OwnRingIoContext context;
  return context.ok() && context.post_resume() && !file.is_open();
}
