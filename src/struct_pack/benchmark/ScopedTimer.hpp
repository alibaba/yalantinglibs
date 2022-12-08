#pragma once
#include <chrono>
#include <iostream>

class ScopedTimer {
 public:
  ScopedTimer(const char *name)
      : m_name(name), m_beg(std::chrono::high_resolution_clock::now()) {}
  ScopedTimer(const char *name, uint64_t &ns) : ScopedTimer(name) {
    m_ns = &ns;
  }
  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_beg);
    if (m_ns)
      *m_ns = dur.count();
    else
      std::cout << m_name << " : " << dur.count() << " ns\n";
  }

 private:
  const char *m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
  uint64_t *m_ns = nullptr;
};
