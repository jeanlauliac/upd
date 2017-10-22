#pragma once

#include "command_line_template.h"
#include "json/parser.h"
#include "manifest/struct.h"
#include "path_glob.h"
#include "substitution.h"

namespace upd {
namespace manifest {

struct unexpected_element_error {};

template <typename RetVal>
struct all_unexpected_elements_handler {
  typedef RetVal return_type;

  template <typename ObjectReader>
  RetVal object(ObjectReader& read_object) const {
    throw unexpected_element_error();
  }

  template <typename ArrayReader>
  RetVal array(ArrayReader& read_array) const {
    throw unexpected_element_error();
  }

  RetVal string_literal(const std::string&) const {
    throw unexpected_element_error();
  }

  RetVal number_literal(float number) const {
    throw unexpected_element_error();
  }
};

/**
 * Push each JSON array elements into an existing `std::vector`, where an
 * instance of `ElementHandler` is used to parse each element of the array.
 */
template <typename ElementHandler>
struct vector_handler: public all_unexpected_elements_handler<void> {
  typedef typename ElementHandler::return_type element_type;
  typedef typename std::vector<element_type> vector_type;

  vector_handler(vector_type& result): result_(result) {}
  vector_handler(vector_type& result, const ElementHandler& handler):
    result_(result), handler_(handler) {}

  template <typename ArrayReader>
  void array(ArrayReader& reader) {
    element_type value;
    while (reader.next(handler_, value)) {
      result_.push_back(value);
    }
  }

private:
  vector_type& result_;
  ElementHandler handler_;
};

template <typename ElementHandler, typename FieldValueReader>
void read_vector_field_value(
  FieldValueReader& read_field_value,
  typename vector_handler<ElementHandler>::vector_type& result
) {
  vector_handler<ElementHandler> handler(result);
  read_field_value.read(handler);
}

struct source_pattern_handler:
  public all_unexpected_elements_handler<path_glob::pattern> {
    path_glob::pattern string_literal(const std::string& pattern_string) const {
      return path_glob::parse(pattern_string);
    }
  };

struct expected_integer_number_error {};

struct read_size_t_handler: public all_unexpected_elements_handler<size_t> {
  size_t number_literal(float number) const {
    size_t result = static_cast<size_t>(number);
    if (static_cast<float>(result) != number) {
      throw expected_integer_number_error();
    }
    return result;
  }
};

struct read_string_handler: public all_unexpected_elements_handler<std::string> {
  std::string string_literal(const std::string& value) const {
    return value;
  }
};

struct read_rule_output_handler: public all_unexpected_elements_handler<substitution::pattern> {
  substitution::pattern string_literal(const std::string& substitution_pattern_string) const {
    return substitution::parse(substitution_pattern_string);
  }
};

struct rule_input_handler:
  public all_unexpected_elements_handler<update_rule_input> {
    template <typename ObjectReader>
    update_rule_input object(ObjectReader& read_object) const {
      update_rule_input input;
      read_object([&input](
        const std::string& field_name,
        typename ObjectReader::field_value_reader& read_field_value
      ) {
        if (field_name == "source_ix") {
          input.input_ix = read_field_value.read(read_size_t_handler());
          input.type = update_rule_input::type::source;
          return;
        }
        if (field_name == "rule_ix") {
          input.input_ix = read_field_value.read(read_size_t_handler());
          input.type = update_rule_input::type::rule;
          return;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      });
      return input;
    }
  };

struct update_rule_handler:
  public all_unexpected_elements_handler<update_rule> {
    template <typename ObjectReader>
    update_rule object(ObjectReader& read_object) const {
      update_rule rule;
      read_object([&rule](
        const std::string& field_name,
        typename ObjectReader::field_value_reader& read_field_value
      ) {
        if (field_name == "command_line_ix") {
          rule.command_line_ix = read_field_value.read(read_size_t_handler());
          return;
        }
        if (field_name == "output") {
          rule.output = read_field_value.read(read_rule_output_handler());
          return;
        }
        if (field_name == "inputs") {
          read_vector_field_value<rule_input_handler>
            (read_field_value, rule.inputs);
          return;
        }
        if (field_name == "dependencies") {
          read_vector_field_value<rule_input_handler>
            (read_field_value, rule.dependencies);
          return;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      });
      return rule;
    }
  };

struct string_handler: public all_unexpected_elements_handler<std::string> {
  std::string string_literal(const std::string& value) const {
    return value;
  }
};

struct command_line_template_variable_handler:
  public all_unexpected_elements_handler<command_line_template_variable> {
    command_line_template_variable string_literal(const std::string& value) {
      if (value == "input_files") {
        return command_line_template_variable::input_files;
      }
      if (value == "output_file") {
        return command_line_template_variable::output_files;
      }
      if (value == "dependency_file") {
        return command_line_template_variable::dependency_file;
      }
      throw std::runtime_error("unknown command line template variable arg");
    }
  };

struct command_line_template_part_handler:
  public all_unexpected_elements_handler<command_line_template_part> {
    template <typename ObjectReader>
    command_line_template_part object(ObjectReader& read_object) const {
      command_line_template_part part;
      read_object([&part](
        const std::string& field_name,
        typename ObjectReader::field_value_reader& read_field_value
      ) {
        if (field_name == "literals") {
          read_vector_field_value<string_handler>
            (read_field_value, part.literal_args);
          return;
        }
        if (field_name == "variables") {
          read_vector_field_value<command_line_template_variable_handler>
            (read_field_value, part.variable_args);
          return;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      });
      return part;
    }
  };

struct command_line_template_handler:
  public all_unexpected_elements_handler<command_line_template> {
    template <typename ObjectReader>
    command_line_template object(ObjectReader& read_object) {
      command_line_template tpl;
      read_object([&tpl](
        const std::string& field_name,
        typename ObjectReader::field_value_reader& read_field_value
      ) {
        if (field_name == "binary_path") {
          tpl.binary_path = read_field_value.read(read_string_handler());
          return;
        }
        if (field_name == "arguments") {
          read_vector_field_value<command_line_template_part_handler>
            (read_field_value, tpl.parts);
          return;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      });
      return tpl;
    }
  };

struct manifest_expression_handler: public all_unexpected_elements_handler<manifest> {
  typedef manifest return_type;

  template <typename ObjectReader>
  manifest object(ObjectReader& read_object) const {
    manifest result;
    read_object([&result](
      const std::string& field_name,
      typename ObjectReader::field_value_reader& read_field_value
    ) {
      if (field_name == "source_patterns") {
        read_vector_field_value<source_pattern_handler>
          (read_field_value, result.source_patterns);
        return;
      }
      if (field_name == "rules") {
        read_vector_field_value<update_rule_handler>
          (read_field_value, result.rules);
        return;
      }
      if (field_name == "command_line_templates") {
        read_vector_field_value<command_line_template_handler>
          (read_field_value, result.command_line_templates);
        return;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    });
    return result;
  }
};

template <typename Lexer>
manifest parse(Lexer& lexer) {
  return json::parse_expression<Lexer, const manifest_expression_handler>
    (lexer, manifest_expression_handler());
}

/**
 * Thrown if we are trying to read the manifest of a project root but it
 * cannot be found.
 */
struct missing_manifest_error {
  missing_manifest_error(const std::string& root_path): root_path(root_path) {}
  std::string root_path;
};

manifest read_file(const std::string& root_path);

}
}
