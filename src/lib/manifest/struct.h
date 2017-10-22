#pragma once

#include "../command_line_template.h"
#include "../path_glob.h"
#include "../substitution.h"

namespace upd {
namespace manifest {

struct update_rule_input {
  enum class type { source, rule };

  update_rule_input() {}
  update_rule_input(type type, size_t input_ix):
    type(type), input_ix(input_ix) {}

  static update_rule_input from_source(size_t ix) {
    return update_rule_input(type::source, ix);
  }

  static update_rule_input from_rule(size_t ix) {
    return update_rule_input(type::rule, ix);
  }

  type type;
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

}
}
