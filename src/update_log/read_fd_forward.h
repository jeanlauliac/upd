#include "../io/io.h"

namespace upd {
namespace update_log {

template <size_t BufferSize> struct read_fd_forward {
  read_fd_forward(int fd) : fd_(fd), next_(buf_.data()), end_(buf_.data()) {}
  size_t operator()(char *buf, size_t count) {
    auto read_count = fill_(buf, count);
    while (read_count < count && read_()) {
      read_count += fill_(buf + read_count, count - read_count);
    }
    return read_count;
  }

private:
  size_t fill_(char *buf, size_t count) {
    count = std::min(count, static_cast<size_t>(end_ - next_));
    std::memcpy(buf, next_, count);
    next_ += count;
    return count;
  }

  bool read_() {
    auto read_count = io::read(fd_, buf_.data(), buf_.size());
    next_ = buf_.data();
    end_ = buf_.data() + read_count;
    return read_count > 0;
  }

  int fd_;
  std::array<char, BufferSize> buf_;
  char *next_;
  char *end_;
};

} // namespace update_log
} // namespace upd
