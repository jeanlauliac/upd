#pragma once

#include "command_line_template.h"
#include "json/parser.h"
#include "path_glob.h"
#include "substitution.h"

namespace upd {
namespace manifest {

enum class update_rule_input_type { source, rule };

struct update_rule_input {
  static update_rule_input from_source(size_t ix) {
    return {
      .type = update_rule_input_type::source,
      .input_ix = ix,
    };
  }

  static update_rule_input from_rule(size_t ix) {
    return {
      .type = update_rule_input_type::rule,
      .input_ix = ix,
    };
  }

  update_rule_input_type type;
  size_t input_ix;
};

inline bool operator==(const update_rule_input& left, const update_rule_input& right) {
  return
    left.type == right.type &&
    left.input_ix == right.input_ix;
}

struct update_rule {
  size_t command_line_ix;
  std::vector<update_rule_input> inputs;
  std::vector<update_rule_input> dependencies;
  substitution::pattern output;
};

inline bool operator==(const update_rule& left, const update_rule& right) {
  return
    left.command_line_ix == right.command_line_ix &&
    left.inputs == right.inputs &&
    left.dependencies == right.dependencies &&
    left.output == right.output;
}

struct manifest {
  std::vector<command_line_template> command_line_templates;
  std::vector<path_glob::pattern> source_patterns;
  std::vector<update_rule> rules;
};

inline bool operator==(const manifest& left, const manifest& right) {
  return
    left.command_line_templates == right.command_line_templates &&
    left.source_patterns == right.source_patterns &&
    left.rules == right.rules;
}

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

template <typename ElementHandler>
struct vector_elements_handler {
  typedef void return_type;
  typedef typename ElementHandler::return_type element_type;
  typedef typename std::vector<element_type> vector_type;

  vector_elements_handler(vector_type& result): result_(result) {}

  template <typename ObjectReader>
  void object(ObjectReader& read_object) {
    result_.push_back(handler_.object(read_object));
  }

  template <typename ArrayReader>
  void array(ArrayReader& read_array) {
    result_.push_back(handler_.array(read_array));
  }

  void string_literal(const std::string& value) {
    result_.push_back(handler_.string_literal(value));
  }

  void number_literal(float number) {
    result_.push_back(handler_.number_literal(number));
  }

private:
  std::vector<element_type>& result_;
  ElementHandler handler_;
};

template <typename ElementHandler>
struct vector_handler: public all_unexpected_elements_handler<void> {
  typedef typename ElementHandler::return_type element_type;
  typedef vector_elements_handler<ElementHandler> elements_handler_type;
  typedef typename elements_handler_type::vector_type vector_type;

  vector_handler(vector_type& result): handler_(result) {}

  template <typename ArrayReader>
  void array(ArrayReader& read_array) {
    read_array(handler_);
  }

private:
  elements_handler_type handler_;
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

struct rule_inputs_array_handler: public all_unexpected_elements_handler<void> {

  template <typename ObjectReader>
  void object(ObjectReader& read_object) {
    update_rule_input input;
    read_object([&input](
      const std::string& field_name,
      typename ObjectReader::field_value_reader& read_field_value
    ) {
      if (field_name == "source_ix") {
        input.input_ix = read_field_value.read(read_size_t_handler());
        input.type = update_rule_input_type::source;
        return;
      }
      if (field_name == "rule_ix") {
        input.input_ix = read_field_value.read(read_size_t_handler());
        input.type = update_rule_input_type::rule;
        return;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    });
    result_.push_back(input);
  }

  const std::vector<update_rule_input>& result() const { return result_; };

private:
  std::vector<update_rule_input> result_;
};

struct rule_inputs_handler: public all_unexpected_elements_handler<std::vector<update_rule_input>> {
  template <typename ArrayReader>
  std::vector<update_rule_input> array(ArrayReader& read_array) const {
    rule_inputs_array_handler handler;
    read_array(handler);
    return handler.result();
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
          rule.inputs = read_field_value.read(rule_inputs_handler());
          return;
        }
        if (field_name == "dependencies") {
          rule.dependencies = read_field_value.read(rule_inputs_handler());
          return;
        }
        throw std::runtime_error("doesn't know field `" + field_name + "`");
      });
      return rule;
    }
  };

struct read_string_array_array_handler: public all_unexpected_elements_handler<void> {
  void string_literal(const std::string& value) {
    result_.push_back(value);
  }

  const std::vector<std::string>& result() const { return result_; };

private:
  std::vector<std::string> result_;
};

struct read_string_array_handler: public all_unexpected_elements_handler<std::vector<std::string>> {
  template <typename ArrayReader>
  std::vector<std::string> array(ArrayReader& read_array) const {
    read_string_array_array_handler handler;
    read_array(handler);
    return handler.result();
  }
};

struct read_variable_arg_array_array_handler: public all_unexpected_elements_handler<void> {
  void string_literal(const std::string& value) {
    if (value == "input_files") {
      result_.push_back(command_line_template_variable::input_files);
      return;
    }
    if (value == "output_file") {
      result_.push_back(command_line_template_variable::output_files);
      return;
    }
    if (value == "dependency_file") {
      result_.push_back(command_line_template_variable::dependency_file);
      return;
    }
    throw std::runtime_error("unknown command line template variable arg");
  }

  const std::vector<command_line_template_variable>& result() const { return result_; };

private:
  std::vector<command_line_template_variable> result_;
};

struct read_variable_arg_array_handler: public all_unexpected_elements_handler<std::vector<command_line_template_variable>> {
  template <typename ArrayReader>
  std::vector<command_line_template_variable> array(ArrayReader& read_array) const {
    read_variable_arg_array_array_handler handler;
    read_array(handler);
    return handler.result();
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
          part.literal_args = read_field_value.read(read_string_array_handler());
          return;
        }
        if (field_name == "variables") {
          part.variable_args = read_field_value.read(read_variable_arg_array_handler());
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
          read_vector_field_value<command_line_template_part_handler>(read_field_value, tpl.parts);
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
        read_vector_field_value<source_pattern_handler>(read_field_value, result.source_patterns);
        return;
      }
      if (field_name == "rules") {
        read_vector_field_value<update_rule_handler>(read_field_value, result.rules);
        return;
      }
      if (field_name == "command_line_templates") {
        read_vector_field_value<command_line_template_handler>(read_field_value, result.command_line_templates);
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

}
}
