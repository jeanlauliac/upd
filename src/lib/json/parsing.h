#pragma once

#include "lexer.h"

namespace upd {
namespace json {

struct unexpected_end_error {};
struct unexpected_string_error {};

enum class unexpected_number_reason {
  field_colon,
  field_name,
  post_item,
  post_field,
  first_field_name,
};

struct unexpected_number_error {
  unexpected_number_error(const location_range &loc_,
                          unexpected_number_reason reason_)
      : location(loc_), reason(reason_) {}

  location_range location;
  unexpected_number_reason reason;
};

enum class unexpected_punctuation_situation {
  expression,
  field_colon,
  field_name,
  first_field_name,
  first_item,
  post_field,
  post_item,
};

struct unexpected_punctuation_error {
  unexpected_punctuation_error(punctuation_type type_, location loc_,
                               unexpected_punctuation_situation situation_)
      : type(type_), location(loc_), situation(situation_) {}

  punctuation_type type;
  location location;
  unexpected_punctuation_situation situation;
};

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer &lexer, Handler &handler);

} // namespace json
} // namespace upd
