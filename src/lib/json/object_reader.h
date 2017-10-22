#pragma once

#include "parsing.h"

namespace upd {
namespace json {

struct read_field_name_handler {
  typedef bool return_type;
  read_field_name_handler(std::string& field_name): field_name_(field_name) {}

  bool end() const { throw unexpected_end_error(); }

  bool punctuation(punctuation_type type) const {
    if (type == punctuation_type::brace_close) return false;
    throw unexpected_punctuation_error();
  }

  bool string_literal(const std::string& literal) const {
    field_name_ = literal;
    return true;
  }

  bool number_literal(float literal) const {
    throw unexpected_number_error();
  }

private:
  std::string& field_name_;
};

struct read_new_field_name_handler {
  typedef bool return_type;
  read_new_field_name_handler(std::string& field_name): field_name_(field_name) {}

  bool end() const { throw unexpected_end_error(); }

  bool punctuation(punctuation_type type) const {
    throw unexpected_punctuation_error();
  }

  bool string_literal(const std::string& literal) const {
    field_name_ = literal;
    return true;
  }

  bool number_literal(float literal) const {
    throw unexpected_number_error();
  }

private:
  std::string& field_name_;
};

struct post_field_handler {
  typedef bool return_type;
  bool end() const { throw unexpected_end_error(); }

  bool punctuation(punctuation_type type) const {
    if (type == punctuation_type::brace_close) return false;
    if (type == punctuation_type::comma) return true;
    throw unexpected_punctuation_error();
  }

  bool string_literal(const std::string& literal) const {
    throw unexpected_string_error();
  }

  bool number_literal(float literal) const {
    throw unexpected_number_error();
  }
};

struct read_field_colon_handler {
  typedef void return_type;
  void end() const { throw unexpected_end_error(); }

  void punctuation(punctuation_type type) const {
    if (type != punctuation_type::colon) {
      throw unexpected_punctuation_error();
    }
  }

  void string_literal(const std::string& literal) const {
    throw unexpected_string_error();
  }

  void number_literal(float literal) const {
    throw unexpected_number_error();
  }
};

template <typename Lexer>
struct field_value_reader {
  field_value_reader(Lexer& lexer): lexer_(lexer) {}

  template <typename Handler>
  typename Handler::return_type read(Handler& handler) {
    return parse_expression(lexer_, handler);
  }

  template <typename Handler>
  typename Handler::return_type read(const Handler& handler) {
    return parse_expression(lexer_, handler);
  }

private:
  Lexer& lexer_;
};

template <typename Lexer>
struct object_reader {
  typedef Lexer lexer_type;
  typedef field_value_reader<Lexer> field_value_reader;

  object_reader(Lexer& lexer): lexer_(lexer) {}

  template <typename FieldReader>
  void operator()(FieldReader read_field) {
    std::string field_name;
    read_field_name_handler rfn_handler(field_name);
    read_field_colon_handler rfc_handler;
    bool has_field = lexer_.next(rfn_handler);
    while (has_field) {
      lexer_.next(rfc_handler);
      field_value_reader read_field_value(lexer_);
      read_field(field_name, read_field_value);
      post_field_handler pf_handler;
      has_field = lexer_.next(pf_handler);
      if (!has_field) continue;
      read_new_field_name_handler rnfn_handler(field_name);
      lexer_.next(rnfn_handler);
    }
  }

private:
  Lexer& lexer_;
};
}
}
