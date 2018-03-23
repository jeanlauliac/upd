#pragma once

#include "../../gen/src/manifest/manifest.h"
#include "../command_line_template.h"
#include "../path_glob.h"
#include "../substitution.h"

namespace upd {
namespace manifest {

struct update_rule_input {

  update_rule_input() {}
  update_rule_input(input_type type_, size_t input_ix_)
      : type(type_), input_ix(input_ix_) {}

  static update_rule_input from_source(size_t ix) {
    return update_rule_input(input_type::source, ix);
  }

  static update_rule_input from_rule(size_t ix) {
    return update_rule_input(input_type::rule, ix);
  }

  input_type type;
  size_t input_ix;
};

inline bool operator==(const update_rule_input &left,
                       const update_rule_input &right) {
  return left.type == right.type && left.input_ix == right.input_ix;
}

inline std::string inspect(const update_rule_input &value,
                           const inspect_options &options) {
  std::ostringstream os;
  inspect_options inner_options({options.global, options.depth + 1});
  os << "upd::manifest::update_rule_input(";
  os << inspect(value.type, inner_options) << ", "
     << inspect(value.input_ix, inner_options) << ')';
  return os.str();
}

struct update_rule {
  size_t command_line_ix;
  std::vector<update_rule_input> inputs;
  std::vector<update_rule_input> order_only_dependencies;
  substitution::pattern output;
};

inline bool operator==(const update_rule &left, const update_rule &right) {
  return left.command_line_ix == right.command_line_ix &&
         left.inputs == right.inputs &&
         left.order_only_dependencies == right.order_only_dependencies &&
         left.output == right.output;
}

inline std::string inspect(const update_rule &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::manifest::update_rule", options);
  insp.push_back("command_line_ix", value.command_line_ix);
  insp.push_back("inputs", value.inputs);
  insp.push_back("order_only_dependencies", value.order_only_dependencies);
  insp.push_back("output", value.output);
  return insp.result();
}

struct manifest {
  std::vector<command_line_template> command_line_templates;
  std::vector<path_glob::pattern> source_patterns;
  std::vector<update_rule> rules;
};

inline bool operator==(const manifest &left, const manifest &right) {
  return left.command_line_templates == right.command_line_templates &&
         left.source_patterns == right.source_patterns &&
         left.rules == right.rules;
}

inline std::string inspect(const manifest &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::manifest::manifest", options);
  insp.push_back("command_line_templates", value.command_line_templates);
  insp.push_back("source_patterns", value.source_patterns);
  insp.push_back("rules", value.rules);
  return insp.result();
}

} // namespace manifest
} // namespace upd
