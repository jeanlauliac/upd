#include "execute_manifest.h"
#include "gen_update_map.h"
#include "manifest.h"
#include "output_dot_graph.h"
#include "output_shell_script.h"
#include "path.h"
#include "update.h"
#include "update_plan.h"

namespace upd {

static const std::string CACHE_FOLDER = ".upd";

void execute_manifest(
  const std::string& root_path,
  const std::string& working_path,
  bool print_graph,
  bool update_all_files,
  const std::vector<std::string>& relative_target_paths,
  bool print_commands,
  bool print_shell_script
) {
  auto manifest = manifest::read_file(root_path);
  const update_map updm = gen_update_map(root_path, manifest);
  const auto& output_files_by_path = updm.output_files_by_path;
  update_plan plan;

  std::vector<std::string> local_target_paths;
  for (auto const& relative_path: relative_target_paths) {
    auto local_target_path = upd::get_local_path(root_path, relative_path, working_path);
    auto target_desc = output_files_by_path.find(local_target_path);
    if (target_desc == output_files_by_path.end()) {
      throw unknown_target_error(relative_path);
    }
    build_update_plan(plan, output_files_by_path, *target_desc);
  }

  if (update_all_files) {
    for (auto const& target_desc: output_files_by_path) {
      build_update_plan(plan, output_files_by_path, target_desc);
    }
  } else if (plan.pending_output_file_paths.size() == 0) {
    throw no_targets_error();
  }
  if (print_graph) {
    output_dot_graph(std::cout, updm, plan, manifest.command_line_templates);
    return;
  }
  if (print_shell_script) {
    output_shell_script(std::cout, updm, plan, manifest.command_line_templates);
    return;
  }

  if (mkdir((root_path + "/" + CACHE_FOLDER).c_str(), 0700) != 0 && errno != EEXIST) {
    throw std::runtime_error("cannot create upd hidden directory");
  }

  std::string log_file_path = root_path + "/" + CACHE_FOLDER + "/log";
  std::string temp_log_file_path = root_path + "/" + CACHE_FOLDER + "/log_rewritten";

  update_context cx = {
    .root_path = root_path,
    .log_cache = update_log::cache::from_log_file(log_file_path),
    .dir_cache = directory_cache<mkdir>(root_path),
    .print_commands = print_commands,
  };
  execute_update_plan(cx, updm, plan, manifest.command_line_templates);

  cx.log_cache.close();
  update_log::rewrite_file(
    log_file_path,
    temp_log_file_path,
    cx.log_cache.records()
  );
}

}
