#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace file_io_test {

inline constexpr size_t block_size = 4096;

inline uint64_t pattern(uint64_t block, size_t word) {
  return (block + 1) * 0x9e3779b97f4a7c15ULL ^
         (word + 1) * 0xd1b54a32d192ed03ULL;
}

class AlignedBuffer {
 public:
  explicit AlignedBuffer(size_t size = block_size) : size_(size) {
    if (::posix_memalign(&data_, block_size, size) != 0) {
      throw std::bad_alloc();
    }
    std::memset(data_, 0, size);
  }
  ~AlignedBuffer() { std::free(data_); }
  AlignedBuffer(const AlignedBuffer &) = delete;
  AlignedBuffer &operator=(const AlignedBuffer &) = delete;
  char *data() { return static_cast<char *>(data_); }
  size_t size() const { return size_; }

 private:
  void *data_ = nullptr;
  size_t size_;
};

inline void fill(char *buffer, uint64_t first_block, size_t size) {
  for (size_t offset = 0; offset < size; offset += sizeof(uint64_t)) {
    auto value = pattern(first_block + offset / block_size,
                         (offset % block_size) / sizeof(uint64_t));
    std::memcpy(buffer + offset, &value, sizeof(value));
  }
}

inline bool verify(const char *buffer, uint64_t first_block, size_t size) {
  for (size_t offset = 0; offset < size; offset += sizeof(uint64_t)) {
    uint64_t value;
    std::memcpy(&value, buffer + offset, sizeof(value));
    if (value != pattern(first_block + offset / block_size,
                         (offset % block_size) / sizeof(uint64_t))) {
      return false;
    }
  }
  return true;
}

class TemporaryFile {
 public:
  explicit TemporaryFile(size_t blocks = 256) : blocks_(blocks) {
    char name[] = "own-ring-data-XXXXXX";
    fd_ = ::mkstemp(name);
    if (fd_ < 0) {
      throw std::system_error(errno, std::system_category(), "mkstemp");
    }
    path_ = name;
    try {
      AlignedBuffer buffer(256 * block_size);
      for (size_t block = 0; block < blocks;) {
        size_t count = std::min<size_t>(256, blocks - block);
        size_t length = count * block_size;
        fill(buffer.data(), block, length);
        size_t done = 0;
        while (done < length) {
          ssize_t result = ::write(fd_, buffer.data() + done, length - done);
          if (result < 0 && errno == EINTR) {
            continue;
          }
          if (result <= 0) {
            throw std::system_error(result < 0 ? errno : EIO,
                                    std::system_category(), "write");
          }
          done += result;
        }
        block += count;
      }
      if (::fsync(fd_) < 0) {
        throw std::system_error(errno, std::system_category(), "fsync");
      }
    } catch (...) {
      ::close(fd_);
      ::unlink(path_.c_str());
      throw;
    }
  }

  ~TemporaryFile() {
    ::close(fd_);
    ::unlink(path_.c_str());
  }
  TemporaryFile(const TemporaryFile &) = delete;
  TemporaryFile &operator=(const TemporaryFile &) = delete;
  const std::string &path() const { return path_; }
  int fd() const { return fd_; }
  size_t blocks() const { return blocks_; }

 private:
  std::string path_;
  int fd_ = -1;
  size_t blocks_;
};

}  // namespace file_io_test
