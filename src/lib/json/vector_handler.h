#pragma once

namespace upd {
namespace json {

struct unexpected_element_error {};

template <typename RetVal> struct all_unexpected_elements_handler {
  typedef RetVal return_type;

  template <typename ObjectReader> RetVal object(ObjectReader &) const {
    throw unexpected_element_error();
  }

  template <typename ArrayReader> RetVal array(ArrayReader &) const {
    throw unexpected_element_error();
  }

  RetVal string_literal(const std::string &) const {
    throw unexpected_element_error();
  }

  RetVal number_literal(float) const { throw unexpected_element_error(); }
};

/**
 * Push each JSON array elements into an existing `std::vector`, where an
 * instance of `ElementHandler` is used to parse each element of the array.
 */
template <typename ElementHandler>
struct vector_handler : public all_unexpected_elements_handler<void> {
  typedef typename ElementHandler::return_type element_type;
  typedef typename std::vector<element_type> vector_type;

  vector_handler(vector_type &result) : result_(result) {}
  vector_handler(vector_type &result, const ElementHandler &handler)
      : result_(result), handler_(handler) {}

  template <typename ArrayReader> void array(ArrayReader &reader) {
    element_type value;
    while (reader.next(handler_, value)) {
      result_.push_back(value);
    }
  }

private:
  vector_type &result_;
  ElementHandler handler_;
};

template <typename ElementHandler, typename ObjectReader>
void read_vector_field_value(
    ObjectReader &object_reader,
    typename json::vector_handler<ElementHandler>::vector_type &result) {
  json::vector_handler<ElementHandler> handler(result);
  object_reader.next_value(handler);
}

} // namespace json
} // namespace upd
