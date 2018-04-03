#pragma once

#include "io.h"

namespace upd {
namespace io {

struct file_descriptor {
  file_descriptor();
  file_descriptor(int fd);
  ~file_descriptor();
  file_descriptor(file_descriptor &other) = delete;
  file_descriptor(file_descriptor &&other);
  file_descriptor &operator=(file_descriptor &) = delete;
  file_descriptor &operator=(file_descriptor &&other);
  operator int() const { return fd_; }
  void close();

private:
  int fd_;
};

} // namespace io
} // namespace upd
