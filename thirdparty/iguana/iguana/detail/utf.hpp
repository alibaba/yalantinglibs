#pragma once

#include <cassert>
#include <stdexcept>

namespace iguana {
// https://github.com/Tencent/rapidjson/blob/master/include/rapidjson/reader.h
template <typename Ch = char, typename It>
inline unsigned parse_unicode_hex4(It &&it) {
  unsigned codepoint = 0;
  for (int i = 0; i < 4; i++) {
    Ch c = *it;
    codepoint <<= 4;
    codepoint += static_cast<unsigned>(c);
    if (c >= '0' && c <= '9')
      codepoint -= '0';
    else if (c >= 'A' && c <= 'F')
      codepoint -= 'A' - 10;
    else if (c >= 'a' && c <= 'f')
      codepoint -= 'a' - 10;
    else {
      throw std::runtime_error("Invalid Unicode Escape Hex");
    }
    ++it;
  }
  return codepoint;
}

// https://github.com/Tencent/rapidjson/blob/master/include/rapidjson/encodings.h
template <typename Ch = char, typename OutputStream>
inline void encode_utf8(OutputStream &os, unsigned codepoint) {
  if (codepoint <= 0x7F)
    os.push_back(static_cast<Ch>(codepoint & 0xFF));
  else if (codepoint <= 0x7FF) {
    os.push_back(static_cast<Ch>(0xC0 | ((codepoint >> 6) & 0xFF)));
    os.push_back(static_cast<Ch>(0x80 | ((codepoint & 0x3F))));
  }
  else if (codepoint <= 0xFFFF) {
    os.push_back(static_cast<Ch>(0xE0 | ((codepoint >> 12) & 0xFF)));
    os.push_back(static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
    os.push_back(static_cast<Ch>(0x80 | (codepoint & 0x3F)));
  }
  else {
    assert(codepoint <= 0x10FFFF);
    os.push_back(static_cast<Ch>(0xF0 | ((codepoint >> 18) & 0xFF)));
    os.push_back(static_cast<Ch>(0x80 | ((codepoint >> 12) & 0x3F)));
    os.push_back(static_cast<Ch>(0x80 | ((codepoint >> 6) & 0x3F)));
    os.push_back(static_cast<Ch>(0x80 | (codepoint & 0x3F)));
  }
}
}  // namespace iguana
