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

struct update_context {
  /**
   * The path of the directory that contains all the files we deal with. Across
   * the code "local" paths are expressed relatively to that root, that must be
   * an absolute path.
   */
  const std::string root_path;

  /**
   * In-memory copy of the "update log", that keeps track of what files we
   * already updated or not. That's loaded from disk on startup and we
   * persist new entries as soon as they arrive, so as to be crash-resilient.
   */
  update_log::cache log_cache;

  /**
   * Keeps track of the known hashes of files. This is useful if, for example,
   * two files are generated from a single source file. In that case we'll
   * want to known the source's hash at two different points in time.
   */
  file_hash_cache hash_cache;

  /**
   * Keeps track of the directories that exist or not. The root path is always
   * assumed to exist. The directory cache is useful to automatically create
   * directories for output files.
   */
  directory_cache<mkdir> dir_cache;

  /**
   * The local path (expressed relative to the root path) of the synthetic
   * "depfile", that must be a FIFO. This is used for rules that do need a
   * C/C++-style depfile to express transitive dependencies.
   */
  const std::string local_depfile_path;

  /**
   * If `true`, commands are printed on the output before they are executed.
   */
  bool print_commands;
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
  update_context& context,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates
);

}
