#pragma once

#include "lexer.h"

struct always_false_handler {
  typedef bool return_type;
  bool end(const upd::json::location &) const { return false; }
  bool punctuation(upd::json::punctuation_type,
                   const upd::json::location &) const {
    return false;
  }
  bool string_literal(const std::string &,
                      const upd::json::location_range_ref &) const {
    return false;
  }
  bool number_literal(float, const upd::json::location_range_ref &) const {
    return false;
  }
};

struct expect_punctuation_handler : public always_false_handler {
  expect_punctuation_handler(upd::json::punctuation_type type_) : type(type_) {}
  bool punctuation(upd::json::punctuation_type that_type,
                   const upd::json::location &) const {
    return that_type == type;
  }
  upd::json::punctuation_type type;
};

struct expect_string_literal_handler : public always_false_handler {
  expect_string_literal_handler(const std::string &literal_)
      : literal(literal_) {}
  bool string_literal(const std::string &that_literal,
                      const upd::json::location_range_ref &) const {
    return that_literal == literal;
  }
  std::string literal;
};

struct expect_number_literal_handler : public always_false_handler {
  expect_number_literal_handler(float literal_) : literal(literal_) {}
  bool number_literal(float that_literal,
                      const upd::json::location_range_ref &) const {
    return that_literal == literal;
  }
  float literal;
};

struct expect_end_handler : public always_false_handler {
  bool end(const upd::json::location &) const { return true; }
};
