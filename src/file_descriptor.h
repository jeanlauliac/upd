#pragma once

#include "io.h"

namespace upd {

struct file_descriptor {
  file_descriptor() : fd_(-1) {}
  file_descriptor(int fd) : fd_(fd) {}
  ~file_descriptor() { close(); }
  file_descriptor(file_descriptor &other) = delete;
  file_descriptor(file_descriptor &&other) : fd_(other.fd_) { other.fd_ = -1; }
  file_descriptor &operator=(file_descriptor &) = delete;
  file_descriptor &operator=(file_descriptor &&other) {
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }
  int fd() const { return fd_; }
  operator int() const { return fd_; }
  void close() {
    if (fd_ >= 0) io::close(fd_);
    fd_ = -1;
  };

private:
  int fd_;
};

} // namespace upd