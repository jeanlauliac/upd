#include "fd_char_reader.h"
#include <stdexcept>
#include <unistd.h>

namespace upd {

bool fd_char_reader::next(char &c) {
  if (next_ >= end_) {
    auto count = read(fd_, buffer_, sizeof(buffer_));
    if (count < 0) {
      throw std::runtime_error("read() failed");
    }
    end_ = buffer_ + count;
    next_ = buffer_;
  }
  if (next_ >= end_) {
    return false;
  }
  c = *next_;
  ++next_;
  return true;
}

} // namespace upd
