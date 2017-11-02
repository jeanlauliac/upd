#pragma once

#include "object_reader.h"
#include "parsing.h"

namespace upd {
namespace json {

template <typename Lexer> struct array_reader {
  typedef Lexer lexer_type;
  enum class state_t { init, reading, done };

  array_reader(Lexer &lexer) : lexer_(lexer), state_(state_t::init) {}

  /**
   * We don't want the reader to be copied so as to ensure the callsite can
   * validate the reading state at the end of the reading operation.
   */
  array_reader(array_reader &) = delete;

  state_t state() const { return state_; }

  template <typename ItemHandler>
  bool next(ItemHandler &item_handler,
            typename ItemHandler::return_type &value) {
    if (state_ == state_t::done) return false;
    if (state_ == state_t::init) {
      typedef first_item_handler<ItemHandler> handler;
      if (lexer_.next(handler(lexer_, item_handler, value))) {
        state_ = state_t::reading;
        return true;
      };
      state_ = state_t::done;
      return false;
    }
    if (!lexer_.next(post_item_handler())) {
      state_ = state_t::done;
      return false;
    }
    value = parse_expression(lexer_, item_handler);
    return true;
  }

private:
  template <typename ItemHandler> struct first_item_handler {
    typedef bool return_type;
    typedef typename ItemHandler::return_type item_type;

    first_item_handler(Lexer &lexer, ItemHandler &item_handler,
                       item_type &value)
        : lexer_(lexer), item_handler_(item_handler), value_(value) {}

    bool end(const location &) const { throw unexpected_end_error(); }

    bool punctuation(punctuation_type type, const location &loc) const {
      if (type == punctuation_type::brace_open) {
        object_reader<Lexer> reader(lexer_);
        value_ = item_handler_.object(reader);
        return true;
      }
      if (type == punctuation_type::bracket_open) {
        array_reader<Lexer> reader(lexer_);
        value_ = item_handler_.array(reader);
        return true;
      }
      if (type == punctuation_type::bracket_close) {
        return false;
      }
      throw unexpected_punctuation_error(
          type, loc, unexpected_punctuation_situation::first_item);
    }

    bool string_literal(const std::string &literal,
                        const location_range_ref &) const {
      value_ = item_handler_.string_literal(literal);
      return true;
    }

    bool number_literal(float literal, const location_range_ref &) const {
      value_ = item_handler_.number_literal(literal);
      return true;
    }

  private:
    Lexer &lexer_;
    ItemHandler &item_handler_;
    item_type &value_;
  };

  struct post_item_handler {
    typedef bool return_type;

    bool end(const location &) const { throw unexpected_end_error(); }

    bool punctuation(punctuation_type type, const location &loc) const {
      if (type == punctuation_type::comma) return true;
      if (type == punctuation_type::bracket_close) return false;
      throw unexpected_punctuation_error(
          type, loc, unexpected_punctuation_situation::post_item);
    }

    bool string_literal(const std::string &literal,
                        const location_range_ref &) const {
      (void)literal;
      throw unexpected_string_error();
    }

    bool number_literal(float literal, const location_range_ref &) const {
      (void)literal;
      throw unexpected_number_error();
    }
  };

private:
  Lexer &lexer_;
  state_t state_;
};

} // namespace json
} // namespace upd
