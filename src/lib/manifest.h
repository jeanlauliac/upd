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

struct source_pattern_array_handler: public all_unexpected_elements_handler<void> {
  source_pattern_array_handler(std::vector<path_glob::pattern>& source_patterns):
    source_patterns_(source_patterns) {}

  void string_literal(const std::string& pattern_string) const {
    source_patterns_.push_back(path_glob::parse(pattern_string));
  }

private:
  std::vector<path_glob::pattern>& source_patterns_;
};

struct source_patterns_handler: public all_unexpected_elements_handler<void> {
  source_patterns_handler(std::vector<path_glob::pattern>& source_patterns):
    source_patterns_(source_patterns) {}

  template <typename ArrayReader>
  void array(ArrayReader& read_array) const {
    source_pattern_array_handler handler(source_patterns_);
    read_array(handler);
  }

private:
  std::vector<path_glob::pattern>& source_patterns_;
};

template <typename Lexer>
void parse_source_patterns(
  json::field_value_reader<Lexer>& read_field_value,
  std::vector<path_glob::pattern>& source_patterns
) {
  source_patterns_handler handler(source_patterns);
  read_field_value.read(handler);
}

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

struct update_rule_array_handler: public all_unexpected_elements_handler<void> {
  update_rule_array_handler(std::vector<update_rule>& rules): rules_(rules) {}

  template <typename ObjectReader>
  void object(ObjectReader& read_object) const {
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
    rules_.push_back(rule);
  }

private:
  std::vector<update_rule>& rules_;
};

struct update_rules_handler: public all_unexpected_elements_handler<void> {
  update_rules_handler(std::vector<update_rule>& rules): rules_(rules) {}

  template <typename ArrayReader>
  void array(ArrayReader& read_array) const {
    update_rule_array_handler handler(rules_);
    read_array(handler);
  }

private:
  std::vector<update_rule>& rules_;
};

template <typename Lexer>
void parse_update_rules(
  json::field_value_reader<Lexer>& read_field_value,
  std::vector<update_rule>& rules
) {
  update_rules_handler handler(rules);
  read_field_value.read(handler);
}

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

struct command_line_segments_array_handler: public all_unexpected_elements_handler<void> {

  template <typename ObjectReader>
  void object(ObjectReader& read_object) {
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
    result_.push_back(part);
  }

  const std::vector<command_line_template_part>& result() const { return result_; };

private:
  std::vector<command_line_template_part> result_;
};

struct command_line_segments_handler: public all_unexpected_elements_handler<std::vector<command_line_template_part>> {
  template <typename ArrayReader>
  std::vector<command_line_template_part> array(ArrayReader& read_array) const {
    command_line_segments_array_handler handler;
    read_array(handler);
    return handler.result();
  }
};

struct command_line_templates_array_handler: public all_unexpected_elements_handler<void> {

  template <typename ObjectReader>
  void object(ObjectReader& read_object) {
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
        tpl.parts = read_field_value.read(command_line_segments_handler());
        return;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    });
    result_.push_back(tpl);
  }

  const std::vector<command_line_template>& result() const { return result_; };

private:
  std::vector<command_line_template> result_;
};

struct command_line_templates_handler: public all_unexpected_elements_handler<std::vector<command_line_template>> {
  template <typename ArrayReader>
  std::vector<command_line_template> array(ArrayReader& read_array) const {
    command_line_templates_array_handler handler;
    read_array(handler);
    return handler.result();
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
        parse_source_patterns(read_field_value, result.source_patterns);
        return;
      }
      if (field_name == "rules") {
        parse_update_rules(read_field_value, result.rules);
        return;
      }
      if (field_name == "command_line_templates") {
        result.command_line_templates =
          read_field_value.read(command_line_templates_handler());
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
