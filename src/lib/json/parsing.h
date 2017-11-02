#pragma once

#include "lexer.h"

namespace upd {
namespace json {

struct unexpected_end_error {};
struct unexpected_number_error {};
struct unexpected_string_error {};

enum class unexpected_punctuation_situation {
  expression,
  field_name,
  first_field_name,
  post_field,
  post_item,
  first_item,
  field_colon,
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
