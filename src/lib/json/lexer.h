#pragma once

#include <iostream>

namespace upd {
namespace json {

enum class punctuation_type {
  brace_close,
  brace_open,
  bracket_close,
  bracket_open,
  colon,
  comma,
};

struct location {
  location() : line(1), column(1) {}

  size_t line;
  size_t column;
};

inline std::ostream &operator<<(std::ostream &os, location loc) {
  return os << loc.line << ':' << loc.column;
}

/**
 * Thrown when it encounters a character that is not part of the JSON grammar.
 */
struct invalid_character_error {
  invalid_character_error(char chr_, location location_)
      : chr(chr_), location(location_) {}

  char chr;
  location location;
};

template <typename CharReader> struct lexer {
  lexer(CharReader &char_reader)
      : char_reader_(char_reader), has_lookahead_(false) {}

  template <typename Handler>
  typename Handler::return_type next(Handler &handler) {
    return this->_next(handler);
  }

  template <typename Handler>
  typename Handler::return_type next(const Handler &handler) {
    return this->_next(handler);
  }

private:
  template <typename Handler>
  typename Handler::return_type _next(Handler &handler) {
    do {
      next_char_();
    } while (good_ && is_whitespace_());
    if (!good_) return handler.end();
    switch (c_) {
    case '[':
      return handler.punctuation(punctuation_type::bracket_open);
    case ']':
      return handler.punctuation(punctuation_type::bracket_close);
    case '{':
      return handler.punctuation(punctuation_type::brace_open);
    case '}':
      return handler.punctuation(punctuation_type::brace_close);
    case ':':
      return handler.punctuation(punctuation_type::colon);
    case ',':
      return handler.punctuation(punctuation_type::comma);
    case '"':
      next_char_();
      return read_string_(handler);
    }
    if (c_ >= '0' && c_ <= '9') {
      return read_number_(handler);
    }
    throw invalid_character_error(c_, loc_);
  }

  template <typename Handler>
  typename Handler::return_type read_string_(Handler &handler) {
    std::string value;
    while (good_ && c_ != '"') {
      if (c_ == '\\') {
        next_char_();
        if (!good_) continue;
      }
      value += c_;
      next_char_();
    }
    if (!good_) throw std::runtime_error("unexpected end in string literal");
    return handler.string_literal(value);
  }

  template <typename Handler>
  typename Handler::return_type read_number_(Handler &handler) {
    float value = 0;
    do {
      value = value * 10 + (c_ - '0');
      next_char_();
    } while (good_ && c_ >= '0' && c_ <= '9');
    has_lookahead_ = true;
    return handler.number_literal(value);
  }

  void next_char_() {
    if (has_lookahead_) {
      has_lookahead_ = false;
      return;
    }
    good_ = char_reader_.next(c_);
    if (!good_) return;
    loc_ = next_loc_;
    if (c_ == '\n') {
      ++next_loc_.line;
      next_loc_.column = 1;
    } else {
      ++next_loc_.column;
    }
  }

  bool is_whitespace_() { return c_ == ' ' || c_ == '\n'; }

  CharReader char_reader_;
  bool has_lookahead_;
  bool good_;
  char c_;
  location loc_;
  location next_loc_;
};

} // namespace json
} // namespace upd
