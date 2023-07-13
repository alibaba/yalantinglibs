#include <ylt/util/time_util.h>

#include <cassert>
#include <iostream>

int main() {
  std::cout << ylt::get_gmt_time_str() << "\n";
  std::cout << ylt::get_local_time_str() << "\n";

  std::string tm_str = "Mon, 02 Jan 2006 15:04:05 GMT";
  if (auto [ok, t] = ylt::get_timestamp(tm_str); ok) {
    std::cout << cinatra::get_gmt_time_str(
                     std::chrono::system_clock::from_time_t(t))
              << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }

  std::string utc_str = "2006-01-02T15:04:05.000Z";
  if (auto [ok, t] = ylt::get_timestamp<ylt::time_format::utc_format>(utc_str);
      ok) {
    std::cout << cinatra::get_gmt_time_str(
                     std::chrono::system_clock::from_time_t(t))
              << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }

  std::string utc_str1 = "20060102T150405000Z";
  if (auto [ok, t] =
          ylt::get_timestamp<ylt::time_format::utc_without_punctuation_format>(
              utc_str1);
      ok) {
    std::cout << cinatra::get_gmt_time_str(
                     std::chrono::system_clock::from_time_t(t))
              << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }
}