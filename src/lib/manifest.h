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
    update_rule_input object(ObjectReader& reader) const {
      update_rule_input input;
      std::string field_name;
      while (reader.next(field_name)) {
        if (field_name == "source_ix") {
          input.input_ix = reader.next_value(read_size_t_handler());
          input.type = update_rule_input::type::source;
          continue;
        }
        if (field_name == "rule_ix") {
          input.input_ix = reader.next_value(read_size_t_handler());
          input.type = update_rule_input::type::rule;
          continue;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      }
      return input;
    }
  };

struct update_rule_handler:
  public all_unexpected_elements_handler<update_rule> {
    template <typename ObjectReader>
    update_rule object(ObjectReader& reader) const {
      update_rule rule;
      std::string field_name;
      while (reader.next(field_name)) {
        if (field_name == "command_line_ix") {
          rule.command_line_ix = reader.next_value(read_size_t_handler());
          continue;
        }
        if (field_name == "output") {
          rule.output = reader.next_value(read_rule_output_handler());
          continue;
        }
        if (field_name == "inputs") {
          vector_handler<rule_input_handler> handler(rule.inputs);
          reader.next_value(handler);
          continue;
        }
        if (field_name == "dependencies") {
          vector_handler<rule_input_handler> handler(rule.dependencies);
          reader.next_value(handler);
          continue;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      }
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
    command_line_template_part object(ObjectReader& reader) const {
      command_line_template_part part;
      std::string field_name;
      while (reader.next(field_name)) {
        if (field_name == "literals") {
          vector_handler<string_handler> handler(part.literal_args);
          reader.next_value(handler);
          continue;
        }
        if (field_name == "variables") {
          vector_handler<command_line_template_variable_handler> handler(part.variable_args);
          reader.next_value(handler);
          continue;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      }
      return part;
    }
  };

struct command_line_template_handler:
  public all_unexpected_elements_handler<command_line_template> {
    template <typename ObjectReader>
    command_line_template object(ObjectReader& reader) {
      command_line_template tpl;
      std::string field_name;
      while (reader.next(field_name)) {
        if (field_name == "binary_path") {
          tpl.binary_path = reader.next_value(read_string_handler());
          continue;
        }
        if (field_name == "arguments") {
          vector_handler<command_line_template_part_handler> handler(tpl.parts);
          reader.next_value(handler);
          continue;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      }
      return tpl;
    }
  };

struct manifest_expression_handler: public all_unexpected_elements_handler<manifest> {
  typedef manifest return_type;

  template <typename ObjectReader>
  manifest object(ObjectReader& reader) const {
    manifest result;
    std::string field_name;
    while (reader.next(field_name)) {
      if (field_name == "source_patterns") {
        vector_handler<source_pattern_handler> handler(result.source_patterns);
        reader.next_value(handler);
        continue;
      }
      if (field_name == "rules") {
        vector_handler<update_rule_handler> handler(result.rules);
        reader.next_value(handler);
        continue;
      }
      if (field_name == "command_line_templates") {
        vector_handler<command_line_template_handler> handler(result.command_line_templates);
        reader.next_value(handler);
        continue;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    }
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

struct invalid_manifest_error {};

manifest read_file(
  const std::string& root_path,
  const std::string& working_path,
  bool use_color
);

}
}
