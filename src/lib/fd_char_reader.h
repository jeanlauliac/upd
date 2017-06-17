#pragma once

namespace upd {

/**
 * We want to read the file descriptor on a character basis, but do some caching
 * to avoid too many I/O calls.
 */
struct fd_char_reader {
  fd_char_reader(int fd): fd_(fd), next_(buffer_), end_(buffer_) {}
  bool next(char& c);

private:
  int fd_;
  char* next_;
  char* end_;
  char buffer_[1 << 12];
};

}
