#pragma once

#include "update.h"
#include "update_plan.h"
#include <vector>

namespace upd {

template <typename OStream>
void output_shell_script(
    OStream &os, const update_map &updm, update_plan &plan,
    std::vector<command_line_template> command_line_templates,
    std::string root_path) {
  std::unordered_set<std::string> mked_dir_paths;
  os << "#!/bin/bash" << std::endl
     << "# generated with `upd --shell-script`" << std::endl
     << "set -ev" << std::endl
     << std::endl;
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);
    auto const &target_file = target_descriptor.second;
    auto const &command_line_tpl =
        command_line_templates[target_file.command_line_ix];
    auto local_dir = dirname(local_target_path);
    if (mked_dir_paths.count(local_dir) == 0) {
      shell_escape(os << "mkdir -p ", local_dir) << std::endl;
      mked_dir_paths.insert(local_dir);
    }
    auto command_line = reify_command_line(command_line_tpl,
                                           {"/dev/null",
                                            target_file.local_input_file_paths,
                                            {local_target_path},
                                            target_file.dependency_groups},
                                           root_path, io::getcwd());
    os << command_line << std::endl;
    plan.erase(local_target_path);
  }
}

} // namespace upd
