#pragma once

#include "array_reader.h"

namespace upd {
namespace json {

template <typename ReturnValue> struct array_body {
  template <typename Lexer, typename Handler>
  static ReturnValue read(array_reader<Lexer> &reader, Handler &handler) {
    ReturnValue value = handler.array(reader);
    if (reader.state() == array_reader<Lexer>::state_t::done) return value;
    throw std::runtime_error(
        "array_reader#next() needs to be called until `false` is returned");
  }
};

template <> struct array_body<void> {
  template <typename Lexer, typename Handler>
  static void read(array_reader<Lexer> &reader, Handler &handler) {
    handler.array(reader);
    if (reader.state() == array_reader<Lexer>::state_t::done) return;
    throw std::runtime_error(
        "array_reader#next() needs to be called until `false` is returned");
  }
};

template <typename Lexer, typename Handler> struct parse_expression_handler {
  typedef typename Handler::return_type return_type;
  parse_expression_handler(Lexer &lexer, Handler &handler)
      : lexer_(lexer), handler_(handler) {}

  return_type end(const location &) const { throw unexpected_end_error(); }

  return_type punctuation(punctuation_type type, const location &loc) const {
    if (type == punctuation_type::brace_open) {
      object_reader<Lexer> reader(lexer_);
      return handler_.object(reader);
    }
    if (type != punctuation_type::bracket_open) {
      throw unexpected_punctuation_error(
          type, loc, unexpected_punctuation_situation::expression);
    }
    array_reader<Lexer> reader(lexer_);
    return array_body<return_type>::read(reader, handler_);
  }

  return_type string_literal(const std::string &literal,
                             const location_range &) const {
    return handler_.string_literal(literal);
  }

  return_type number_literal(float literal, const location_range &) const {
    return handler_.number_literal(literal);
  }

private:
  Lexer &lexer_;
  Handler &handler_;
};

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer &lexer, Handler &handler) {
  parse_expression_handler<Lexer, Handler> lx_handler(lexer, handler);
  return lexer.next(lx_handler);
}

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer &lexer,
                                               const Handler &handler) {
  parse_expression_handler<Lexer, const Handler> lx_handler(lexer, handler);
  return lexer.next(lx_handler);
}

} // namespace json
} // namespace upd
