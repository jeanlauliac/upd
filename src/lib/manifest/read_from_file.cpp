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

template <typename ReturnValue,
          template <typename ObjectReader> class FieldReader>
struct object_handler
    : public json::all_unexpected_elements_handler<ReturnValue> {
  template <typename ObjectReader>
  ReturnValue object(ObjectReader &reader) const {
    ReturnValue value;
    std::string field_name;
    while (reader.next(field_name)) {
      FieldReader<ObjectReader &>::read(reader, field_name, value);
    }
    return value;
  }
};

template <typename ObjectReader> struct read_rule_input_field {
  static void read(ObjectReader reader, const std::string &field_name,
                   update_rule_input &value) {
    if (field_name == "source_ix") {
      value.input_ix = reader.next_value(read_size_t_handler());
      value.type = update_rule_input::type::source;
      return;
    }
    if (field_name == "rule_ix") {
      value.input_ix = reader.next_value(read_size_t_handler());
      value.type = update_rule_input::type::rule;
      return;
    }
    throw std::runtime_error("doesn't know field `" + field_name + "`");
  }
};

template <typename ObjectReader> struct read_rule_field {
  static void read(ObjectReader reader, const std::string &field_name,
                   update_rule &value) {
    if (field_name == "command_line_ix") {
      value.command_line_ix = reader.next_value(read_size_t_handler());
      return;
    }
    if (field_name == "output") {
      value.output = reader.next_value(read_rule_output_handler());
      return;
    }
    if (field_name == "inputs") {
      json::read_vector_field_value<
          object_handler<update_rule_input, read_rule_input_field>>(
          reader, value.inputs);
      return;
    }
    if (field_name == "dependencies" ||
        field_name == "order_only_dependencies") {
      json::read_vector_field_value<
          object_handler<update_rule_input, read_rule_input_field>>(
          reader, value.order_only_dependencies);
      return;
    }
    throw std::runtime_error("doesn't know field `" + field_name + "`");
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

template <typename ObjectReader> struct read_command_line_template_part_field {
  static void read(ObjectReader reader, const std::string &field_name,
                   command_line_template_part &value) {
    if (field_name == "literals") {
      json::read_vector_field_value<string_handler>(reader, value.literal_args);
      return;
    }
    if (field_name == "variables") {
      json::read_vector_field_value<command_line_template_variable_handler>(
          reader, value.variable_args);
      return;
    }
    throw std::runtime_error("doesn't know field `" + field_name + "`");
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
        json::read_vector_field_value<object_handler<
            command_line_template_part, read_command_line_template_part_field>>(
            reader, tpl.parts);
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
        json::read_vector_field_value<source_pattern_handler>(
            reader, result.source_patterns);
        continue;
      }
      if (field_name == "rules") {
        json::read_vector_field_value<
            object_handler<update_rule, read_rule_field>>(reader, result.rules);
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
