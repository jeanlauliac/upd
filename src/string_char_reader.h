#pragma once

#include <string>

namespace upd {

struct string_char_reader {
  string_char_reader(std::string str) : str_(str), next_index_(0) {}

  bool next(char &c) {
    if (next_index_ >= str_.size()) return false;
    c = str_[next_index_++];
    return true;
  }

private:
  std::string str_;
  size_t next_index_;
};

} // namespace upd
