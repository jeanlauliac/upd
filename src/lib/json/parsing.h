#pragma once

#include "lexer.h"

namespace upd {
namespace json {

struct unexpected_end_error {};
struct unexpected_number_error {};
struct unexpected_string_error {};
struct unexpected_punctuation_error {};

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer &lexer, Handler &handler);

} // namespace json
} // namespace upd
