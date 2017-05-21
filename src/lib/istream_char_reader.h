#pragma once

namespace upd {

/**
 * We want to read the stream on a character basis, but do some caching to avoid
 * too many I/O calls. We use templating to avoid using virtual functions as
 * much as possible.
 */
template <typename istream_t>
struct istream_char_reader {
  istream_char_reader(istream_t& stream):
    stream_(stream), next_(buffer_), end_(buffer_) {}

  bool next(char& c) {
    if (next_ >= end_) {
      stream_.read(buffer_, sizeof(buffer_));
      end_ = buffer_ + stream_.gcount();
      next_ = buffer_;
    }
    if (next_ >= end_) {
      return false;
    }
    c = *next_;
    ++next_;
    return true;
  }

private:
  istream_t& stream_;
  char* next_;
  char* end_;
  char buffer_[1 << 12];
};

}
