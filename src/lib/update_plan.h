#pragma once

#include "command_line_template.h"
#include "update.h"
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace upd {

/**
 * At any point during the update, the plan describes the work left to do.
 */
struct update_plan {
  /**
   * Remove a file from the plan, for example because we finished updating it
   * succesfully. This potentially allows descendants to be available for
   * update.
   */
  void erase(const std::string& local_target_path) {
    pending_output_file_paths.erase(local_target_path);
    auto descendants_iter = descendants_by_path.find(local_target_path);
    if (descendants_iter == descendants_by_path.end()) {
      return;
    }
    for (auto const& descendant_path: descendants_iter->second) {
      auto count_iter = pending_input_counts_by_path.find(descendant_path);
      if (count_iter == pending_input_counts_by_path.end()) {
        throw std::runtime_error("update plan is corrupted");
      }
      int& input_count = count_iter->second;
      --input_count;
      if (input_count == 0) {
        queued_output_file_paths.push(descendant_path);
      }
    }
  }

  /**
   * The paths of all the output files that are ready to be updated immediately.
   * These files' dependencies either have already been updated, or they are
   * source files written manually.
   */
  std::queue<std::string> queued_output_file_paths;

  /**
   * All the files that remain to update.
   */
  std::unordered_set<std::string> pending_output_file_paths;

  /**
   * For each output file path, indicates how many input files still need to be
   * updated before the output file can be updated.
   */
  std::unordered_map<std::string, int> pending_input_counts_by_path;

  /**
   * For each input file path, indicates what files could potentially be
   * updated after the input file is updated.
   */
  std::unordered_map<std::string, std::vector<std::string>> descendants_by_path;
};

bool build_update_plan_for_path(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::string& local_target_path,
  const std::string& local_input_path
);

void build_update_plan(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::pair<std::string, output_file>& target_descriptor
);

void execute_update_plan(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates,
  const std::string& local_depfile_path,
  bool print_commands,
  directory_cache<mkdir>& dir_cache
);

}
