#pragma once

#include "lexer.h"

namespace upd {
namespace json {

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer& lexer, Handler& handler);

struct unexpected_end_error {};
struct unexpected_number_error {};
struct unexpected_string_error {};
struct unexpected_punctuation_error {};

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

template <typename Lexer>
struct array_reader;

template <typename Lexer, typename ItemHandler>
struct read_array_first_item_handler {
  typedef bool return_type;
  read_array_first_item_handler(Lexer& lexer, ItemHandler& item_handler):
    lexer_(lexer), item_handler_(item_handler) {}

  bool end() const { throw unexpected_end_error(); }

  bool punctuation(punctuation_type type) const {
    if (type == punctuation_type::brace_open) {
      object_reader<Lexer> reader(lexer_);
      item_handler_.object(reader);
      return true;
    }
    if (type == punctuation_type::bracket_open) {
      array_reader<Lexer> reader(lexer_);
      item_handler_.array(reader);
      return true;
    }
    if (type == punctuation_type::bracket_close) {
      return false;
    }
    throw unexpected_punctuation_error();
  }

  bool string_literal(const std::string& literal) const {
    item_handler_.string_literal(literal);
    return true;
  }

  bool number_literal(float literal) const {
    item_handler_.number_literal(literal);
    return true;
  }

private:
  Lexer& lexer_;
  ItemHandler& item_handler_;
};

struct array_post_item_handler {
  typedef bool return_type;

  bool end() const { throw unexpected_end_error(); }

  bool punctuation(punctuation_type type) const {
    if (type == punctuation_type::comma) return true;
    if (type == punctuation_type::bracket_close) return false;
    throw unexpected_punctuation_error();
  }

  bool string_literal(const std::string& literal) const {
    throw unexpected_string_error();
  }

  bool number_literal(float literal) const {
    throw unexpected_number_error();
  }
};

template <typename Lexer>
struct array_reader {
  typedef Lexer lexer_type;
  array_reader(Lexer& lexer): lexer_(lexer) {}

  template <typename ItemHandler>
  void operator()(ItemHandler& item_handler) {
    typedef read_array_first_item_handler<Lexer, ItemHandler> first_item_handler;
    bool has_more_items = lexer_.next(first_item_handler(lexer_, item_handler));
    if (!has_more_items) return;
    has_more_items = lexer_.next(array_post_item_handler());
    while (has_more_items) {
      parse_expression(lexer_, item_handler);
      has_more_items = lexer_.next(array_post_item_handler());
    };
  }

private:
  Lexer& lexer_;
};

template <typename Lexer, typename Handler>
struct parse_expression_handler {
  typedef typename Handler::return_type return_type;
  parse_expression_handler(Lexer& lexer, Handler& handler):
    lexer_(lexer), handler_(handler) {}

  return_type end() const {
    throw unexpected_end_error();
  }

  return_type punctuation(punctuation_type type) const {
    if (type == punctuation_type::brace_open) {
      object_reader<Lexer> reader(lexer_);
      return handler_.object(reader);
    }
    if (type == punctuation_type::bracket_open) {
      array_reader<Lexer> reader(lexer_);
      return handler_.array(reader);
    }
    throw unexpected_punctuation_error();
  }

  return_type string_literal(const std::string& literal) const {
    return handler_.string_literal(literal);
  }

  return_type number_literal(float literal) const {
    return handler_.number_literal(literal);
  }

private:
  Lexer& lexer_;
  Handler& handler_;
};

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer& lexer, Handler& handler) {
  parse_expression_handler<Lexer, Handler> lx_handler(lexer, handler);
  return lexer.next(lx_handler);
}

template <typename Lexer, typename Handler>
typename Handler::return_type parse_expression(Lexer& lexer, const Handler& handler) {
  parse_expression_handler<Lexer, const Handler> lx_handler(lexer, handler);
  return lexer.next(lx_handler);
}

}
}
