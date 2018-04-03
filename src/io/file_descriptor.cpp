#include "file_descriptor.h"

namespace upd {
namespace io {

file_descriptor::file_descriptor() : fd_(-1) {}
file_descriptor::file_descriptor(int fd) : fd_(fd) {}
file_descriptor::~file_descriptor() { close(); }

file_descriptor::file_descriptor(file_descriptor &&other) : fd_(other.fd_) {
  other.fd_ = -1;
}

file_descriptor &file_descriptor::operator=(file_descriptor &&other) {
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}

void file_descriptor::close() {
  if (fd_ >= 0) io::close(fd_);
  fd_ = -1;
};

} // namespace io
} // namespace upd
