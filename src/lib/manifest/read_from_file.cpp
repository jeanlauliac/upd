#include "read_from_file.h"

#include "../cli/utils.h"
#include "../io.h"
#include "../istream_char_reader.h"
#include "../json/vector_handler.h"
#include "../path.h"
#include <fstream>

namespace upd {
namespace manifest {

struct source_pattern_handler
    : public json::all_unexpected_elements_handler<path_glob::pattern> {
  path_glob::pattern string_literal(const std::string &pattern_string) const {
    return path_glob::parse(pattern_string);
  }
};

struct expected_integer_number_error {};

struct read_size_t_handler
    : public json::all_unexpected_elements_handler<size_t> {
  size_t number_literal(float number) const {
    size_t result = static_cast<size_t>(number);
    if (static_cast<float>(result) != number) {
      throw expected_integer_number_error();
    }
    return result;
  }
};

struct read_string_handler
    : public json::all_unexpected_elements_handler<std::string> {
  std::string string_literal(const std::string &value) const { return value; }
};

struct read_rule_output_handler
    : public json::all_unexpected_elements_handler<substitution::pattern> {
  substitution::pattern
  string_literal(const std::string &substitution_pattern_string) const {
    return substitution::parse(substitution_pattern_string);
  }
};

struct rule_input_handler
    : public json::all_unexpected_elements_handler<update_rule_input> {
  template <typename ObjectReader>
  update_rule_input object(ObjectReader &reader) const {
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

struct update_rule_handler
    : public json::all_unexpected_elements_handler<update_rule> {
  template <typename ObjectReader>
  update_rule object(ObjectReader &reader) const {
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
        json::read_vector_field_value<rule_input_handler>(reader, rule.inputs);
        continue;
      }
      if (field_name == "dependencies" ||
          field_name == "order_only_dependencies") {
        json::read_vector_field_value<rule_input_handler>(
            reader, rule.order_only_dependencies);
        continue;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    }
    return rule;
  }
};

struct string_handler
    : public json::all_unexpected_elements_handler<std::string> {
  std::string string_literal(const std::string &value) const { return value; }
};

struct command_line_template_variable_handler
    : public json::all_unexpected_elements_handler<
          command_line_template_variable> {
  command_line_template_variable string_literal(const std::string &value) {
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

struct command_line_template_part_handler
    : public json::all_unexpected_elements_handler<command_line_template_part> {
  template <typename ObjectReader>
  command_line_template_part object(ObjectReader &reader) const {
    command_line_template_part part;
    std::string field_name;
    while (reader.next(field_name)) {
      if (field_name == "literals") {
        json::read_vector_field_value<string_handler>(reader, part.literal_args);
        continue;
      }
      if (field_name == "variables") {
        json::read_vector_field_value<command_line_template_variable_handler>(
            reader, part.variable_args);
        continue;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    }
    return part;
  }
};

struct command_line_template_handler
    : public json::all_unexpected_elements_handler<command_line_template> {
  template <typename ObjectReader>
  command_line_template object(ObjectReader &reader) {
    command_line_template tpl;
    std::string field_name;
    while (reader.next(field_name)) {
      if (field_name == "binary_path") {
        tpl.binary_path = reader.next_value(read_string_handler());
        continue;
      }
      if (field_name == "arguments") {
        json::read_vector_field_value<command_line_template_part_handler>(reader,
                                                                    tpl.parts);
        continue;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    }
    return tpl;
  }
};

struct manifest_expression_handler
    : public json::all_unexpected_elements_handler<manifest> {
  typedef manifest return_type;

  template <typename ObjectReader> manifest object(ObjectReader &reader) const {
    manifest result;
    std::string field_name;
    while (reader.next(field_name)) {
      if (field_name == "source_patterns") {
        json::read_vector_field_value<source_pattern_handler>(reader,
                                                        result.source_patterns);
        continue;
      }
      if (field_name == "rules") {
        json::read_vector_field_value<update_rule_handler>(reader, result.rules);
        continue;
      }
      if (field_name == "command_line_templates") {
        json::read_vector_field_value<command_line_template_handler>(
            reader, result.command_line_templates);
        continue;
      }
      throw std::runtime_error("doesn't know field `" + field_name + "`");
    }
    return result;
  }
};

template <typename Lexer> manifest parse(Lexer &lexer) {
  return json::parse_expression<Lexer, const manifest_expression_handler>(
      lexer, manifest_expression_handler());
}

template manifest parse(string_lexer &);

manifest read_from_file(const std::string &root_path) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  std::string file_path = root_path + io::UPDFILE_SUFFIX;
  file.open(file_path);
  if (!file.is_open()) {
    throw missing_manifest_error(root_path);
  }
  istream_char_reader<std::ifstream> reader(file);
  json::lexer<istream_char_reader<std::ifstream>> lexer(reader);
  try {
    return parse(lexer);
  } catch (json::invalid_character_error error) {
    throw invalid_manifest_error<json::invalid_character_error>{file_path,
                                                                error};
  } catch (json::unexpected_punctuation_error error) {
    throw invalid_manifest_error<json::unexpected_punctuation_error>{file_path,
                                                                     error};
  } catch (json::unexpected_number_error error) {
    throw invalid_manifest_error<json::unexpected_number_error>{file_path,
                                                                error};
  }
}

} // namespace manifest
} // namespace upd
