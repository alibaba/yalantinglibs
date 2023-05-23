#include <util/time_util.h>

#include <cassert>

int main() {
  std::string tm_str = "Mon, 02 Jan 2006 15:04:05 GMT";
  if (auto [ok, t] = ylt::get_timestamp(tm_str); ok) {
    std::cout << cinatra::get_gmt_time_str(t) << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }

  std::string utc_str = "2006-01-02T15:04:05.000Z";
  if (auto [ok, t] = ylt::get_timestamp<ylt::time_format::utc_format>(utc_str);
      ok) {
    std::cout << cinatra::get_gmt_time_str(t) << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }

  std::string utc_str1 = "20060102T150405000Z";
  if (auto [ok, t] =
          ylt::get_timestamp<ylt::time_format::utc_without_punctuation_format>(
              utc_str1);
      ok) {
    std::cout << cinatra::get_gmt_time_str(t) << "\n";
    assert(ylt::get_gmt_time_str(t) == tm_str);
  }
}