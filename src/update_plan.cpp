#include "update_plan.h"

namespace upd {

bool build_update_plan_for_path(
    update_plan &plan,
    const std::unordered_map<std::string, output_file> &output_files_by_path,
    const std::string &local_target_path, const std::string &local_input_path) {
  auto input_descriptor = output_files_by_path.find(local_input_path);
  if (input_descriptor == output_files_by_path.end()) return false;
  plan.descendants_by_path[local_input_path].push_back(local_target_path);
  build_update_plan(plan, output_files_by_path, *input_descriptor);
  return true;
}

void build_update_plan(
    update_plan &plan,
    const std::unordered_map<std::string, output_file> &output_files_by_path,
    const std::pair<std::string, output_file> &target_descriptor) {
  auto local_target_path = target_descriptor.first;
  auto pending = plan.pending_output_file_paths.find(local_target_path);
  // FIXME: is this actually useful?
  if (pending != plan.pending_output_file_paths.end()) return;
  plan.pending_output_file_paths.insert(local_target_path);
  int input_count = 0;
  for (auto const &local_input_path :
       target_descriptor.second.local_input_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path,
                                   local_target_path, local_input_path))
      input_count++;
  }
  for (auto const &group : target_descriptor.second.dependency_groups) {
    for (auto const &path : group) {
      if (build_update_plan_for_path(plan, output_files_by_path,
                                     local_target_path, path))
        input_count++;
    }
  }
  for (auto const &local_dependency_path :
       target_descriptor.second.order_only_dependency_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path,
                                   local_target_path, local_dependency_path))
      input_count++;
  }
  if (input_count == 0)
    plan.queued_output_file_paths.push(local_target_path);
  else
    plan.pending_input_counts_by_path[local_target_path] = input_count;
}

struct worker_state {
  worker_state(std::mutex &mutex, std::condition_variable &cv)
      : status(worker_status::idle), cli_template(nullptr),
        local_src_paths(nullptr), order_only_dep_file_paths(nullptr),
        worker(status, result, sfu.job, mutex, cv) {}
  worker_state(worker_state &) = delete;
  worker_state(worker_state &&other) = delete;

  worker_status status;
  command_line_result result;
  scheduled_file_update sfu;
  std::string local_target_path;
  const command_line_template *cli_template;
  const std::vector<std::string> *local_src_paths;
  const std::vector<std::vector<std::string>> *dep_groups;
  const std::unordered_set<std::string> *order_only_dep_file_paths;
  update_worker worker;
};

struct worker_pool {
  worker_pool() : lock(state_mutex){};
  ~worker_pool();

  std::mutex state_mutex;
  std::unique_lock<std::mutex> lock;
  std::condition_variable global_cv;
  std::vector<std::unique_ptr<worker_state>> worker_states;
};

/**
 * We want to make sure to shutdown the threads cleanly, otherwise
 * `std::thread::~thread` will call `std::terminate`.
 */
worker_pool::~worker_pool() {
  if (!lock.owns_lock()) lock.lock();
  for (auto &ws : worker_states) {
    ws->status = worker_status::shutdown;
    ws->worker.notify();
  }
  lock.unlock();
  for (auto &ws : worker_states) {
    ws->worker.join();
  }
}

void execute_update_plan(
    update_context &cx, const update_map &updm, update_plan &plan,
    std::vector<command_line_template> command_line_templates) {

  worker_pool pool;
  std::vector<std::unique_ptr<worker_state>> &worker_states =
      pool.worker_states;

  while (!plan.pending_output_file_paths.empty()) {
    while (!plan.queued_output_file_paths.empty()) {
      auto local_target_path = plan.queued_output_file_paths.front();
      auto &target_descriptor =
          *updm.output_files_by_path.find(local_target_path);
      auto const &target_file = target_descriptor.second;
      auto const &command_line_tpl =
          command_line_templates[target_file.command_line_ix];
      auto const &local_src_paths = target_file.local_input_file_paths;
      if (is_file_up_to_date(cx.log_cache, cx.hash_cache, cx.root_path,
                             local_target_path, local_src_paths,
                             target_file.dependency_groups, command_line_tpl)) {
        plan.queued_output_file_paths.pop();
        plan.erase(local_target_path);
        continue;
      }

      size_t i = 0;
      while (i < worker_states.size() &&
             worker_states[i]->status != worker_status::idle)
        ++i;
      if (i == worker_states.size()) {
        if (i >= cx.concurrency) break;
        auto wr =
            std::make_unique<worker_state>(pool.state_mutex, pool.global_cv);
        worker_states.push_back(std::move(wr));
      }
      plan.queued_output_file_paths.pop();

      auto &st = *worker_states[i];
      st.sfu = schedule_file_update(cx, command_line_tpl, local_src_paths,
                                    local_target_path,
                                    target_file.dependency_groups);
      st.cli_template = &command_line_tpl;
      st.local_src_paths = &local_src_paths;
      st.dep_groups = &target_file.dependency_groups;
      st.local_target_path = std::move(local_target_path);
      st.order_only_dep_file_paths =
          &target_file.order_only_dependency_file_paths;
      st.status = worker_status::in_progress;
      st.worker.notify();
    }

    bool has_errors = false;
    bool has_in_progress;
    do {
      do {
        has_in_progress = false;
        bool has_finished = false;
        for (size_t i = 0; i < worker_states.size(); ++i) {
          if (worker_states[i]->status == worker_status::in_progress)
            has_in_progress = true;
          if (worker_states[i]->status == worker_status::finished)
            has_finished = true;
        }
        if (has_finished || !has_in_progress) break;
        pool.global_cv.wait(pool.lock);
      } while (true);

      for (size_t i = 0; i < worker_states.size(); ++i) {
        if (worker_states[i]->status != worker_status::finished) continue;
        auto &st = *worker_states[i];
        st.status = worker_status::idle;

        std::cerr << st.result.stderr;

        bool has_error = false;
        if (st.result.stdout.size() > 0) {
          std::cerr
              << "upd: error: process has unexpected output on stdout"
              << std::endl
              << "Update commands are not allowed to produce output except "
              << "diagnostics on strerr." << std::endl
              << "========= STDOUT =========" << std::endl
              << st.result.stdout << "========= END =========" << std::endl;
          has_error = true;
        }
        auto exit_code = WEXITSTATUS(st.result.status);
        if (WIFEXITED(st.result.status) == 0) {
          std::cerr << "upd: error: process did not exit normally" << std::endl;
          has_error = true;
          if (WIFSIGNALED(st.result.status) != 0) {
            std::cerr << "upd: error: process exited by signal "
                      << WTERMSIG(st.result.status) << std::endl;
          }
        } else if (exit_code != 0) {
          std::cerr << "upd: error: process terminated with exit code "
                    << exit_code << std::endl;
          has_error = true;
        }
        if (has_error) {
          has_errors = true;
          // FIXME: we still need to remove the depfile FIFO and stuff that
          // is done by `finalize_scheduled_update`
          continue;
        }

        finalize_scheduled_update(
            cx, st.sfu, *st.cli_template, *st.local_src_paths, *st.dep_groups,
            st.local_target_path, updm, *st.order_only_dep_file_paths);
        plan.erase(st.local_target_path);
      }
    } while (has_errors && has_in_progress);

    if (has_errors) {
      break;
    }
  }
}

} // namespace upd
