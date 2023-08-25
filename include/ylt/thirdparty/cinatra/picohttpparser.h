/*
 * Copyright (c) 2009-2014 Kazuho Oku, Tokuhiro Matsuno, Daisuke Murase,
 *                         Shigeo Mitsunari
 *
 * The software is licensed under either the MIT License (below) or the Perl
 * license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef picohttpparser_h
#define picohttpparser_h
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <string_view>

#ifdef _MSC_VER
#define ssize_t intptr_t
#endif

namespace cinatra {
struct http_header {
  std::string_view name;
  std::string_view value;
};
namespace detail {

/* contains name and value of a header (name == NULL if is a continuing line
 * of a multiline header */

/* returns number of bytes consumed if successful, -2 if request is partial,
 * -1 if failed */
// int phr_parse_request(const char *buf, size_t len, const char **method,
// size_t *method_len, const char **path, size_t *path_len,
//                      int *minor_version, struct phr_header *headers, size_t
//                      *num_headers, size_t last_len);

/* ditto */
// int phr_parse_response(const char *_buf, size_t len, int *minor_version, int
// *status, const char **msg, size_t *msg_len,
//                       struct phr_header *headers, size_t *num_headers, size_t
//                       last_len);

/* ditto */
// int phr_parse_headers(const char *buf, size_t len, struct phr_header
// *headers, size_t *num_headers, size_t last_len);

/* should be zero-filled before start */
struct phr_chunked_decoder {
  size_t bytes_left_in_chunk; /* number of bytes left in current chunk */
  char consume_trailer;       /* if trailing headers should be consumed */
  char _hex_count;
  char _state;
};

/* the function rewrites the buffer given as (buf, bufsz) removing the chunked-
 * encoding headers.  When the function returns without an error, bufsz is
 * updated to the length of the decoded data available.  Applications should
 * repeatedly call the function while it returns -2 (incomplete) every time
 * supplying newly arrived data.  If the end of the chunked-encoded data is
 * found, the function returns a non-negative number indicating the number of
 * octets left undecoded at the tail of the supplied buffer.  Returns -1 on
 * error.
 */
// ssize_t phr_decode_chunked(struct phr_chunked_decoder *decoder, char *buf,
// size_t *bufsz);

/* returns if the chunked decoder is in middle of chunked data */
// int phr_decode_chunked_is_in_data(struct phr_chunked_decoder *decoder);

/* $Id: a707070d11d499609f99d09f97535642cec910a8 $ */

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#ifdef _MSC_VER
#define ALIGNED(n) _declspec(align(n))
#else
#define ALIGNED(n) __attribute__((aligned(n)))
#endif

#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

#define CHECK_EOF()     \
  if (buf == buf_end) { \
    *ret = -2;          \
    return NULL;        \
  }

#define EXPECT_CHAR_NO_CHECK(ch) \
  if (*buf++ != ch) {            \
    *ret = -1;                   \
    return NULL;                 \
  }

#define EXPECT_CHAR(ch) \
  CHECK_EOF();          \
  EXPECT_CHAR_NO_CHECK(ch);

#define ADVANCE_TOKEN(tok, toklen)                                            \
  do {                                                                        \
    const char *tok_start = buf;                                              \
    static const char ALIGNED(16) ranges2[] = "\000\040\177\177";             \
    int found2;                                                               \
    buf = findchar_fast(buf, buf_end, ranges2, sizeof(ranges2) - 1, &found2); \
    if (!found2) {                                                            \
      CHECK_EOF();                                                            \
    }                                                                         \
    while (1) {                                                               \
      if (*buf == ' ') {                                                      \
        break;                                                                \
      }                                                                       \
      else if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {                         \
        if ((unsigned char)*buf < '\040' || *buf == '\177') {                 \
          *ret = -1;                                                          \
          return NULL;                                                        \
        }                                                                     \
      }                                                                       \
      ++buf;                                                                  \
      CHECK_EOF();                                                            \
    }                                                                         \
    tok = tok_start;                                                          \
    toklen = buf - tok_start;                                                 \
  } while (0)

static const char *token_char_map =
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static const char *findchar_fast(const char *buf, const char *buf_end,
                                 const char *ranges, int ranges_size,
                                 int *found) {
  *found = 0;
  /* suppress unused parameter warning */
  (void)buf_end;
  (void)ranges;
  (void)ranges_size;
  return buf;
}

static const char *get_token_to_eol(const char *buf, const char *buf_end,
                                    const char **token, size_t *token_len,
                                    int *ret) {
  const char *token_start = buf;

  /* find non-printable char within the next 8 bytes, this is the hottest code;
   * manually inlined */
  while (likely(buf_end - buf >= 8)) {
#define DOIT()                               \
  do {                                       \
    if (unlikely(!IS_PRINTABLE_ASCII(*buf))) \
      goto NonPrintable;                     \
    ++buf;                                   \
  } while (0)
    DOIT();
    DOIT();
    DOIT();
    DOIT();
    DOIT();
    DOIT();
    DOIT();
    DOIT();
#undef DOIT
    continue;
  NonPrintable:
    if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) ||
        unlikely(*buf == '\177')) {
      goto FOUND_CTL;
    }
    ++buf;
  }

  for (;; ++buf) {
    CHECK_EOF();
    if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {
      if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) ||
          unlikely(*buf == '\177')) {
        goto FOUND_CTL;
      }
    }
  }
FOUND_CTL:
  if (likely(*buf == '\015')) {
    ++buf;
    EXPECT_CHAR('\012');
    *token_len = buf - 2 - token_start;
  }
  else if (*buf == '\012') {
    *token_len = buf - token_start;
    ++buf;
  }
  else {
    *ret = -1;
    return NULL;
  }
  *token = token_start;

  return buf;
}

static const char *is_complete(const char *buf, const char *buf_end,
                               size_t last_len, int *ret) {
  int ret_cnt = 0;
  buf = last_len < 3 ? buf : buf + last_len - 3;

  while (1) {
    CHECK_EOF();
    if (*buf == '\015') {
      ++buf;
      CHECK_EOF();
      EXPECT_CHAR('\012');
      ++ret_cnt;
    }
    else if (*buf == '\012') {
      ++buf;
      ++ret_cnt;
    }
    else {
      ++buf;
      ret_cnt = 0;
    }
    if (ret_cnt == 2) {
      return buf;
    }
  }

  *ret = -2;
  return NULL;
}

#define PARSE_INT(valp_, mul_)    \
  if (*buf < '0' || '9' < *buf) { \
    buf++;                        \
    *ret = -1;                    \
    return NULL;                  \
  }                               \
  *(valp_) = (mul_) * (*buf++ - '0');

#define PARSE_INT_3(valp_) \
  do {                     \
    int res_ = 0;          \
    PARSE_INT(&res_, 100)  \
    *valp_ = res_;         \
    PARSE_INT(&res_, 10)   \
    *valp_ += res_;        \
    PARSE_INT(&res_, 1)    \
    *valp_ += res_;        \
  } while (0)

/* returned pointer is always within [buf, buf_end), or null */
static const char *parse_http_version(const char *buf, const char *buf_end,
                                      int *minor_version, int *ret) {
  /* we want at least [HTTP/1.<two chars>] to try to parse */
  if (buf_end - buf < 9) {
    *ret = -2;
    return NULL;
  }
  EXPECT_CHAR_NO_CHECK('H');
  EXPECT_CHAR_NO_CHECK('T');
  EXPECT_CHAR_NO_CHECK('T');
  EXPECT_CHAR_NO_CHECK('P');
  EXPECT_CHAR_NO_CHECK('/');
  EXPECT_CHAR_NO_CHECK('1');
  EXPECT_CHAR_NO_CHECK('.');
  PARSE_INT(minor_version, 1);
  return buf;
}

static const char *parse_headers(const char *buf, const char *buf_end,
                                 http_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret) {
  for (;; ++*num_headers) {
    const char *name;
    size_t name_len;
    const char *value;
    size_t value_len;
    CHECK_EOF();
    if (*buf == '\015') {
      ++buf;
      EXPECT_CHAR('\012');
      break;
    }
    else if (*buf == '\012') {
      ++buf;
      break;
    }
    if (*num_headers == max_headers) {
      *ret = -1;
      return NULL;
    }
    if (!(*num_headers != 0 && (*buf == ' ' || *buf == '\t'))) {
      /* parsing name, but do not discard SP before colon, see
       * http://www.mozilla.org/security/announce/2006/mfsa2006-33.html */
      name = buf;
      static const char ALIGNED(16) ranges1[] =
          "\x00 "  /* control chars and up to SP */
          "\"\""   /* 0x22 */
          "()"     /* 0x28,0x29 */
          ",,"     /* 0x2c */
          "//"     /* 0x2f */
          ":@"     /* 0x3a-0x40 */
          "[]"     /* 0x5b-0x5d */
          "{\377"; /* 0x7b-0xff */
      int found;
      buf = findchar_fast(buf, buf_end, ranges1, sizeof(ranges1) - 1, &found);
      if (!found) {
        CHECK_EOF();
      }
      while (1) {
        if (*buf == ':') {
          break;
        }
        else if (!token_char_map[(unsigned char)*buf]) {
          *ret = -1;
          return NULL;
        }
        ++buf;
        CHECK_EOF();
      }
      if ((name_len = buf - name) == 0) {
        *ret = -1;
        return NULL;
      }
      ++buf;
      for (;; ++buf) {
        CHECK_EOF();
        if (!(*buf == ' ' || *buf == '\t')) {
          break;
        }
      }
    }
    else {
      name = NULL;
      name_len = 0;
    }
    if ((buf = get_token_to_eol(buf, buf_end, &value, &value_len, ret)) ==
        NULL) {
      return NULL;
    }
    headers[*num_headers] = {std::string_view{name, name_len},
                             std::string_view{value, value_len}};
  }
  return buf;
}

static const char *parse_request(const char *buf, const char *buf_end,
                                 const char **method, size_t *method_len,
                                 const char **path, size_t *path_len,
                                 int *minor_version, http_header *headers,
                                 size_t *num_headers, size_t max_headers,
                                 int *ret) {
  /* skip first empty line (some clients add CRLF after POST content) */
  CHECK_EOF();
  if (*buf == '\015') {
    ++buf;
    EXPECT_CHAR('\012');
  }
  else if (*buf == '\012') {
    ++buf;
  }

  /* parse request line */
  ADVANCE_TOKEN(*method, *method_len);
  ++buf;
  ADVANCE_TOKEN(*path, *path_len);
  ++buf;
  if ((buf = parse_http_version(buf, buf_end, minor_version, ret)) == NULL) {
    return NULL;
  }
  if (*buf == '\015') {
    ++buf;
    EXPECT_CHAR('\012');
  }
  else if (*buf == '\012') {
    ++buf;
  }
  else {
    *ret = -1;
    return NULL;
  }

  return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

inline int phr_parse_request(const char *buf_start, size_t len,
                             const char **method, size_t *method_len,
                             const char **path, size_t *path_len,
                             int *minor_version, http_header *headers,
                             size_t *num_headers, size_t last_len) {
  const char *buf = buf_start, *buf_end = buf_start + len;
  size_t max_headers = *num_headers;
  int r;

  *method = NULL;
  *method_len = 0;
  *path = NULL;
  *path_len = 0;
  *minor_version = -1;
  *num_headers = 0;

  /* if last_len != 0, check if the request is complete (a fast countermeasure
  againt slowloris */
  if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
    return r;
  }

  if ((buf = parse_request(buf + last_len, buf_end, method, method_len, path,
                           path_len, minor_version, headers, num_headers,
                           max_headers, &r)) == NULL) {
    return r;
  }

  return (int)(buf - buf_start - last_len);
}

inline const char *parse_response(const char *buf, const char *buf_end,
                                  int *minor_version, int *status,
                                  const char **msg, size_t *msg_len,
                                  http_header *headers, size_t *num_headers,
                                  size_t max_headers, int *ret) {
  /* parse "HTTP/1.x" */
  if ((buf = parse_http_version(buf, buf_end, minor_version, ret)) == NULL) {
    return NULL;
  }
  /* skip space */
  if (*buf++ != ' ') {
    *ret = -1;
    return NULL;
  }
  /* parse status code, we want at least [:digit:][:digit:][:digit:]<other char>
   * to try to parse */
  if (buf_end - buf < 4) {
    *ret = -2;
    return NULL;
  }
  PARSE_INT_3(status);

  /* skip space */
  if (*buf++ != ' ') {
    *ret = -1;
    return NULL;
  }
  /* get message */
  if ((buf = get_token_to_eol(buf, buf_end, msg, msg_len, ret)) == NULL) {
    return NULL;
  }

  return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

inline int phr_parse_response(const char *buf_start, size_t len,
                              int *minor_version, int *status, const char **msg,
                              size_t *msg_len, http_header *headers,
                              size_t *num_headers, size_t last_len) {
  const char *buf = buf_start, *buf_end = buf + len;
  size_t max_headers = *num_headers;
  int r;

  *minor_version = -1;
  *status = 0;
  *msg = NULL;
  *msg_len = 0;
  *num_headers = 0;

  /* if last_len != 0, check if the response is complete (a fast countermeasure
  against slowloris */
  if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
    return r;
  }

  if ((buf = parse_response(buf, buf_end, minor_version, status, msg, msg_len,
                            headers, num_headers, max_headers, &r)) == NULL) {
    return r;
  }

  return (int)(buf - buf_start);
}

inline int phr_parse_headers(const char *buf_start, size_t len,
                             http_header *headers, size_t *num_headers,
                             size_t last_len) {
  const char *buf = buf_start, *buf_end = buf + len;
  size_t max_headers = *num_headers;
  int r;

  *num_headers = 0;

  /* if last_len != 0, check if the response is complete (a fast countermeasure
  against slowloris */
  if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
    return r;
  }

  if ((buf = parse_headers(buf, buf_end, headers, num_headers, max_headers,
                           &r)) == NULL) {
    return r;
  }

  return (int)(buf - buf_start);
}

enum {
  CHUNKED_IN_CHUNK_SIZE,
  CHUNKED_IN_CHUNK_EXT,
  CHUNKED_IN_CHUNK_DATA,
  CHUNKED_IN_CHUNK_CRLF,
  CHUNKED_IN_TRAILERS_LINE_HEAD,
  CHUNKED_IN_TRAILERS_LINE_MIDDLE
};

static int decode_hex(int ch) {
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  }
  else if ('A' <= ch && ch <= 'F') {
    return ch - 'A' + 0xa;
  }
  else if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 0xa;
  }
  else {
    return -1;
  }
}

inline ssize_t phr_decode_chunked(struct phr_chunked_decoder *decoder,
                                  char *buf, size_t *_bufsz) {
  size_t dst = 0, src = 0, bufsz = *_bufsz;
  ssize_t ret = -2; /* incomplete */

  while (1) {
    switch (decoder->_state) {
      case CHUNKED_IN_CHUNK_SIZE:
        for (;; ++src) {
          int v;
          if (src == bufsz)
            goto Exit;
          if ((v = decode_hex(buf[src])) == -1) {
            if (decoder->_hex_count == 0) {
              ret = -1;
              goto Exit;
            }
            break;
          }
          if (decoder->_hex_count == sizeof(size_t) * 2) {
            ret = -1;
            goto Exit;
          }
          decoder->bytes_left_in_chunk = decoder->bytes_left_in_chunk * 16 + v;
          ++decoder->_hex_count;
        }
        decoder->_hex_count = 0;
        decoder->_state = CHUNKED_IN_CHUNK_EXT;
        /* fallthru */
      case CHUNKED_IN_CHUNK_EXT:
        /* RFC 7230 A.2 "Line folding in chunk extensions is disallowed" */
        for (;; ++src) {
          if (src == bufsz)
            goto Exit;
          if (buf[src] == '\012')
            break;
        }
        ++src;
        if (decoder->bytes_left_in_chunk == 0) {
          if (decoder->consume_trailer) {
            decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
            break;
          }
          else {
            goto Complete;
          }
        }
        decoder->_state = CHUNKED_IN_CHUNK_DATA;
        /* fallthru */
      case CHUNKED_IN_CHUNK_DATA: {
        size_t avail = bufsz - src;
        if (avail < decoder->bytes_left_in_chunk) {
          if (dst != src)
            memmove(buf + dst, buf + src, avail);
          src += avail;
          dst += avail;
          decoder->bytes_left_in_chunk -= avail;
          goto Exit;
        }
        if (dst != src)
          memmove(buf + dst, buf + src, decoder->bytes_left_in_chunk);
        src += decoder->bytes_left_in_chunk;
        dst += decoder->bytes_left_in_chunk;
        decoder->bytes_left_in_chunk = 0;
        decoder->_state = CHUNKED_IN_CHUNK_CRLF;
      }
        /* fallthru */
      case CHUNKED_IN_CHUNK_CRLF:
        for (;; ++src) {
          if (src == bufsz)
            goto Exit;
          if (buf[src] != '\015')
            break;
        }
        if (buf[src] != '\012') {
          ret = -1;
          goto Exit;
        }
        ++src;
        decoder->_state = CHUNKED_IN_CHUNK_SIZE;
        break;
      case CHUNKED_IN_TRAILERS_LINE_HEAD:
        for (;; ++src) {
          if (src == bufsz)
            goto Exit;
          if (buf[src] != '\015')
            break;
        }
        if (buf[src++] == '\012')
          goto Complete;
        decoder->_state = CHUNKED_IN_TRAILERS_LINE_MIDDLE;
        /* fallthru */
      case CHUNKED_IN_TRAILERS_LINE_MIDDLE:
        for (;; ++src) {
          if (src == bufsz)
            goto Exit;
          if (buf[src] == '\012')
            break;
        }
        ++src;
        decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
        break;
      default:
        assert(!"decoder is corrupt");
    }
  }

Complete:
  ret = bufsz - src;
Exit:
  if (dst != src)
    memmove(buf + dst, buf + src, bufsz - src);
  *_bufsz = dst;
  return ret;
}

inline int phr_decode_chunked_is_in_data(struct phr_chunked_decoder *decoder) {
  return decoder->_state == CHUNKED_IN_CHUNK_DATA;
}
}  // namespace detail
}  // namespace cinatra
#undef CHECK_EOF
#undef EXPECT_CHAR
#undef ADVANCE_TOKEN

#endif
