#pragma once
#include "read.h"
#include <sstream>

namespace upd {
namespace depfile {

/**
 * Transform a stream of characters into tokens. We have to look one character
 * ahead to know when a token ends. For example a string is ended when we know
 * the next character is space.
 */
template <typename CharReader> struct tokenizer {
  tokenizer(CharReader &char_reader) : char_reader_(char_reader) { read_(); }

  template <typename handler_t, typename retval_t>
  retval_t next(handler_t &handler) {
    while (good_ && (c_ == ' ')) read_();
    if (!good_) {
      return handler.end();
    };
    if (c_ == ':') {
      read_();
      return handler.colon();
    }
    if (c_ == '\n') {
      read_();
      return handler.new_line();
    }
    std::ostringstream oss;
    // TODO: refactor as a `do...while`, condition is always true for first pass
    while (good_ && !(c_ == ' ' || c_ == ':' || c_ == '\n')) {
      if (c_ == '\\') {
        read_();
      }
      oss.put(c_);
      read_();
    }
    auto str = oss.str();
    if (str.size() == 0) {
      throw std::runtime_error("string token of size zero, parser is broken");
    }
    return handler.string(str);
  }

private:
  /**
   * In depfiles, the backslashes escapes anything, including syntax characters.
   * To escape, for example, spaces in a file path, 2 backslashes are required.
   * Consider a file "path with spaces/foo.cpp":
   *
   *     foo.o: path\\ with\\ spaces/foo.cpp
   *
   * This escaping behavior technically allows having newlines in file names,
   * with "\\<LF>". Whereas "\<LF>" just converts the significant newline into
   * whitespace.
   */
  void read_() {
    read_unescaped_();
    if (!good_ || c_ != '\\') return;
    read_unescaped_();
    if (!good_) {
      throw std::runtime_error("expected character after escape sequence `\\`");
    }
    if (c_ == '\n') {
      c_ = ' ';
    }
  }
  void read_unescaped_() { good_ = char_reader_.next(c_); }
  char c_;
  bool good_;
  CharReader char_reader_;
};

} // namespace depfile
} // namespace upd
