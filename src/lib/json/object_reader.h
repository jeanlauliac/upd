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
struct object_reader {
  typedef Lexer lexer_type;
  enum struct state_t { init, reading, done };

  object_reader(Lexer& lexer): lexer_(lexer), state_(state_t::init) {}

  /**
   * Read the next field, return `true` if there is a field, `false` if we
   * reached the end of the object. `field_name` contains the name of the
   * field if it returned `true`.
   */
  bool next(std::string& field_name) {
    if (state_ == state_t::reading) {
      post_field_handler pf_handler;
      if (!lexer_.next(pf_handler)) {
        state_ = state_t::done;
        return false;
      }
    }
    if (state_ == state_t::done) return false;
    bool has_field;
    if (state_ == state_t::init) {
      read_field_name_handler rfn_handler(field_name);
      has_field = lexer_.next(rfn_handler);
    } else {
      read_new_field_name_handler rnfn_handler(field_name);
      has_field = lexer_.next(rnfn_handler);
    }
    if (!has_field) {
      state_ = state_t::done;
      return false;
    }
    state_ = state_t::reading;
    read_field_colon_handler rfc_handler;
    lexer_.next(rfc_handler);
    return true;
  }

  template <typename Handler>
  typename Handler::return_type next_value(Handler& handler) {
    return parse_expression(lexer_, handler);
  }

  template <typename Handler>
  typename Handler::return_type next_value(const Handler& handler) {
    return parse_expression(lexer_, handler);
  }

private:
  Lexer& lexer_;
  state_t state_;
};

}
}
