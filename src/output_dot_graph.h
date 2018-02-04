#pragma once

#include "update.h"
#include "update_plan.h"
#include <vector>

namespace upd {

template <typename OStream>
void output_dot_graph(
    OStream &os, const update_map &updm, update_plan &plan,
    std::vector<command_line_template> command_line_templates) {
  os << "# generated with `upd --dot-graph`" << std::endl
     << "digraph upd {" << std::endl
     << "  rankdir=\"LR\";" << std::endl;
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);
    auto const &target_file = target_descriptor.second;
    auto const &command_line_tpl =
        command_line_templates[target_file.command_line_ix];
    for (auto const &input_path : target_file.local_input_file_paths) {
      os << "  \"" << input_path << "\" -> \"" << local_target_path
         << "\" [label=\"" << command_line_tpl.binary_path << "\"];"
         << std::endl;
    }

    plan.erase(local_target_path);
  }
  os << "}" << std::endl;
}

} // namespace upd
