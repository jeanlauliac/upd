#pragma once

#include <string>
#include <vector>

namespace upd {

struct unknown_target_error {
  unknown_target_error(const std::string& relative_path):
    relative_path(relative_path) {}
  std::string relative_path;
};

struct no_targets_error {};

struct update_failed_error {};

void execute_manifest(
  const std::string& root_path,
  const std::string& working_path,
  bool print_graph,
  bool update_all_files,
  const std::vector<std::string>& relative_target_paths,
  bool print_commands,
  bool print_shell_script,
  size_t concurrency
);

}
