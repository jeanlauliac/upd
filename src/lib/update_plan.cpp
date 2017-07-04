#include "update_plan.h"

namespace upd {

bool build_update_plan_for_path(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::string& local_target_path,
  const std::string& local_input_path
) {
  auto input_descriptor = output_files_by_path.find(local_input_path);
  if (input_descriptor == output_files_by_path.end()) return false;
  plan.descendants_by_path[local_input_path].push_back(local_target_path);
  build_update_plan(plan, output_files_by_path, *input_descriptor);
  return true;
}

void build_update_plan(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::pair<std::string, output_file>& target_descriptor
) {
  auto local_target_path = target_descriptor.first;
  auto pending = plan.pending_output_file_paths.find(local_target_path);
  if (pending != plan.pending_output_file_paths.end()) {
    return;
  }
  plan.pending_output_file_paths.insert(local_target_path);
  int input_count = 0;
  for (auto const& local_input_path: target_descriptor.second.local_input_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path, local_target_path, local_input_path)) {
      input_count++;
    }
  }
  for (auto const& local_dependency_path: target_descriptor.second.local_dependency_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path, local_target_path, local_dependency_path)) {
      input_count++;
    }
  }
  if (input_count == 0) {
    plan.queued_output_file_paths.push(local_target_path);
  } else {
    plan.pending_input_counts_by_path[local_target_path] = input_count;
  }
}

void execute_update_plan(
  update_context& cx,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates
) {
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);
    auto target_file = target_descriptor.second;
    auto const& command_line_tpl = command_line_templates[target_file.command_line_ix];
    auto const& local_src_paths = target_file.local_input_file_paths;
    if (!is_file_up_to_date(cx.log_cache, cx.hash_cache, cx.root_path, local_target_path, local_src_paths, command_line_tpl)) {
      auto sfu = schedule_file_update(
        cx,
        command_line_tpl,
        local_src_paths,
        local_target_path
      );
      cx.worker.wait();
      finalize_scheduled_update(
        cx,
        sfu,
        command_line_tpl,
        local_src_paths,
        local_target_path,
        updm,
        target_file.local_dependency_file_paths
      );
    }
    plan.erase(local_target_path);
  }
}

}
