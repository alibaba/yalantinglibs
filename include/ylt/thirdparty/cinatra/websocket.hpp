#pragma once
#include "utils.hpp"
#include "ws_define.h"

namespace cinatra {
enum ws_header_status {
  error = -1,
  complete = 0,
  incomplete = -2,
};
class websocket {
 public:
  void sec_ws_key(std::string_view sec_key) { sec_ws_key_ = sec_key; }

  std::string_view get_sec_ws_key() { return sec_ws_key_; }

  /*
  0               1               2               3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-------+-+-------------+-------------------------------+
  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
  | |1|2|3|       |K|             |                               |
  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
  |     Extended payload length continued, if payload len == 127  |
  + - - - - - - - - - - - - - - - +-------------------------------+
  |                               |Masking-key, if MASK set to 1  |
  +-------------------------------+-------------------------------+
  | Masking-key (continued)       |          Payload Data         |
  +-------------------------------- - - - - - - - - - - - - - - - +
  :                     Payload Data continued ...                :
  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
  |                     Payload Data continued ...                |
  +---------------------------------------------------------------+
  opcode:
  *  %x0 denotes a continuation frame
  *  %x1 denotes a text frame
  *  %x2 denotes a binary frame
  *  %x3-7 are reserved for further non-control frames
  *  %x8 denotes a connection close
  *  %x9 denotes a ping
  *  %xA denotes a pong
  *  %xB-F are reserved for further control frames
  Payload length:  7 bits, 7+16 bits, or 7+64 bits
  Masking-key:  0 or 4 bytes
  */
  ws_header_status parse_header(const char *buf, size_t size,
                                bool is_server = true) {
    const unsigned char *inp = (const unsigned char *)(buf);

    msg_opcode_ = inp[0] & 0x0F;
    msg_fin_ = (inp[0] >> 7) & 0x01;
    unsigned char msg_masked = (inp[1] >> 7) & 0x01;

    int pos = 2;
    int length_field = inp[1] & (~0x80);

    left_header_len_ = 0;
    if (length_field <= 125) {
      len_bytes_ = SHORT_HEADER;
      payload_length_ = length_field;
    }
    else if (length_field == 126)  // msglen is 16bit!
    {
      len_bytes_ = MEDIUM_HEADER;
      payload_length_ = ntohs(*(uint16_t *)&inp[2]);  // (inp[2] << 8) + inp[3];
      pos += 2;
      left_header_len_ =
          is_server ? MEDIUM_HEADER - size : CLIENT_MEDIUM_HEADER - size;
    }
    else if (length_field == 127)  // msglen is 64bit!
    {
      len_bytes_ = LONG_HEADER;
      payload_length_ = (size_t)be64toh(*(uint64_t *)&inp[2]);
      pos += 8;
      left_header_len_ =
          is_server ? LONG_HEADER - size : CLIENT_LONG_HEADER - size;
    }
    else {
      len_bytes_ = INVALID_HEADER;
      return ws_header_status::error;
    }

    if (msg_masked) {
      std::memcpy(mask_, inp + pos, 4);
    }

    return left_header_len_ == 0 ? ws_header_status::complete
                                 : ws_header_status::incomplete;
  }

  int len_bytes() const { return len_bytes_; }
  void reset_len_bytes() { len_bytes_ = SHORT_HEADER; }

  ws_frame_type parse_payload(std::span<char> buf) {
    // unmask data:
    if (*(uint32_t *)mask_ != 0) {
      for (size_t i = 0; i < payload_length_; i++) {
        buf[i] = buf[i] ^ mask_[i % 4];
      }
    }

    if (msg_opcode_ == 0x0)
      return (msg_fin_)
                 ? ws_frame_type::WS_TEXT_FRAME
                 : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;  // continuation
    // frame ?
    if (msg_opcode_ == 0x1)
      return (msg_fin_) ? ws_frame_type::WS_TEXT_FRAME
                        : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;
    if (msg_opcode_ == 0x2)
      return (msg_fin_) ? ws_frame_type::WS_BINARY_FRAME
                        : ws_frame_type::WS_INCOMPLETE_BINARY_FRAME;
    if (msg_opcode_ == 0x8)
      return ws_frame_type::WS_CLOSE_FRAME;
    if (msg_opcode_ == 0x9)
      return ws_frame_type::WS_PING_FRAME;
    if (msg_opcode_ == 0xA)
      return ws_frame_type::WS_PONG_FRAME;
    return ws_frame_type::WS_BINARY_FRAME;
  }

  ws_frame_type parse_payload(const char *buf, size_t size,
                              std::string &outbuf) {
    const unsigned char *inp = (const unsigned char *)(buf);
    if (payload_length_ > size)
      return ws_frame_type::WS_INCOMPLETE_FRAME;

    if (payload_length_ > outbuf.size()) {
      outbuf.resize((size_t)payload_length_);
    }

    if (*(uint32_t *)mask_ == 0) {
      memcpy(&outbuf[0], (void *)(inp), payload_length_);
    }
    else {
      // unmask data:
      for (size_t i = 0; i < payload_length_; i++) {
        outbuf[i] = inp[i] ^ mask_[i % 4];
      }
    }

    if (msg_opcode_ == 0x0)
      return (msg_fin_)
                 ? ws_frame_type::WS_TEXT_FRAME
                 : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;  // continuation
                                                             // frame ?
    if (msg_opcode_ == 0x1)
      return (msg_fin_) ? ws_frame_type::WS_TEXT_FRAME
                        : ws_frame_type::WS_INCOMPLETE_TEXT_FRAME;
    if (msg_opcode_ == 0x2)
      return (msg_fin_) ? ws_frame_type::WS_BINARY_FRAME
                        : ws_frame_type::WS_INCOMPLETE_BINARY_FRAME;
    if (msg_opcode_ == 0x8)
      return ws_frame_type::WS_CLOSE_FRAME;
    if (msg_opcode_ == 0x9)
      return ws_frame_type::WS_PING_FRAME;
    if (msg_opcode_ == 0xA)
      return ws_frame_type::WS_PONG_FRAME;
    return ws_frame_type::WS_BINARY_FRAME;
  }

  std::string format_header(size_t length, opcode code) {
    size_t header_length = encode_header(length, code);
    return {msg_header_, header_length};
  }

  std::vector<asio::const_buffer> format_message(const char *src, size_t length,
                                                 opcode code) {
    size_t header_length = encode_header(length, code);
    return {asio::buffer(msg_header_, header_length),
            asio::buffer(src, length)};
  }

  std::string encode_frame(std::span<char> &data, opcode op, bool need_mask,
                           bool eof = true) {
    std::string header;
    /// Base header.
    frame_header hdr{};
    hdr.fin = eof;
    hdr.rsv1 = 0;
    hdr.rsv2 = 0;
    hdr.rsv3 = 0;
    hdr.opcode = static_cast<uint8_t>(op);
    hdr.mask = 1;

    if (data.empty()) {
      int mask = 0;
      header.resize(sizeof(frame_header) + sizeof(mask));
      std::memcpy(header.data(), &hdr, sizeof(hdr));
      std::memcpy(header.data() + sizeof(hdr), &mask, sizeof(mask));
      return header;
    }

    hdr.len =
        data.size() < 126 ? data.size() : (data.size() < 65536 ? 126 : 127);

    uint8_t buffer[sizeof(frame_header)];
    std::memcpy(buffer, (uint8_t *)&hdr, sizeof(hdr));
    std::string str_hdr_len =
        std::string((const char *)buffer, sizeof(frame_header));
    header.append(str_hdr_len);

    /// The payload length may be larger than 126 bytes.
    std::string str_payload_len;
    if (data.size() >= 126) {
      if (data.size() >= 65536) {
        uint64_t len = data.size();
        str_payload_len.resize(sizeof(uint64_t));
        *((uint64_t *)&str_payload_len[0]) = htobe64(len);
      }
      else {
        uint16_t len = data.size();
        str_payload_len.resize(sizeof(uint16_t));
        *((uint16_t *)&str_payload_len[0]) = htons(static_cast<uint16_t>(len));
      }
      header.append(str_payload_len);
    }

    /// The mask is a 32-bit value.
    uint8_t mask[4] = {};
    if (need_mask) {
      header[1] |= 0x80;
      uint32_t random = (uint32_t)rand();
      memcpy(mask, &random, 4);
    }

    size_t size = header.size();
    header.resize(size + 4);
    std::memcpy(header.data() + size, mask, 4);

    for (int i = 0; i < data.size(); ++i) {
      data[i] ^= mask[i % 4];
    }

    return header;
  }

  close_frame parse_close_payload(char *src, size_t length) {
    close_frame cf = {};
    if (length >= 2) {
      std::memcpy(&cf.code, src, 2);
      cf = {ntohs(cf.code), src + 2, length - 2};
      if (cf.code < 1000 || cf.code > 4999 ||
          (cf.code > 1011 && cf.code < 4000) ||
          (cf.code >= 1004 && cf.code <= 1006) ||
          !is_valid_utf8((unsigned char *)cf.message, cf.length)) {
        return {};
      }
    }
    return cf;
  }

  std::string format_close_payload(uint16_t code, char *message,
                                   size_t length) {
    if (length == 0) {
      return "";
    }
    std::string close_payload;
    if (code) {
      close_payload.resize(length + 2);
      code = htons(code);
      std::memcpy(close_payload.data(), &code, 2);
      std::memcpy(close_payload.data() + 2, message, length);
    }
    return close_payload;
  }

  size_t left_header_len() const { return left_header_len_; }

  size_t payload_length() const { return payload_length_; }

  opcode get_opcode() { return (opcode)msg_opcode_; }

 private:
  size_t encode_header(size_t length, opcode code) {
    size_t header_length;

    if (length < 126) {
      header_length = 2;
      msg_header_[1] = static_cast<char>(length);
    }
    else if (length <= UINT16_MAX) {
      header_length = 4;
      msg_header_[1] = 126;
      *((uint16_t *)&msg_header_[2]) = htons(static_cast<uint16_t>(length));
    }
    else {
      header_length = 10;
      msg_header_[1] = 127;
      *((uint64_t *)&msg_header_[2]) = htobe64(length);
    }

    int flags = 0;
    msg_header_[0] = (flags & SND_NO_FIN ? 0 : char(128));
    if (!(flags & SND_CONTINUATION)) {
      msg_header_[0] |= code;
    }

    return header_length;
  }

  std::string_view sec_ws_key_;

  size_t payload_length_ = 0;
  size_t left_payload_length_ = 0;

  size_t left_header_len_ = 0;
  uint8_t mask_[4] = {};
  unsigned char msg_opcode_ = 0;
  unsigned char msg_fin_ = 0;

  char msg_header_[10];
  ws_head_len len_bytes_ = SHORT_HEADER;
};

}  // namespace cinatra
