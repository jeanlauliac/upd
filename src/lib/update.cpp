#include "command_line_template.h"
#include "depfile.h"
#include "update.h"

namespace upd {

XXH64_hash_t hash_command_line(const command_line& command_line) {
  xxhash64_stream cli_hash(0);
  cli_hash << upd::hash(command_line.binary_path);
  cli_hash << upd::hash(command_line.args);
  return cli_hash.digest();
}

XXH64_hash_t hash_files(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_paths
) {
  xxhash64_stream imprint_s(0);
  for (auto const& local_path: local_paths) {
    imprint_s << hash_cache.hash(root_path + '/' + local_path);
  }
  return imprint_s.digest();
}

XXH64_hash_t get_target_imprint(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_src_paths,
  std::vector<std::string> dependency_local_paths,
  const command_line& command_line
) {
  xxhash64_stream imprint_s(0);
  imprint_s << hash_command_line(command_line);
  imprint_s << hash_files(hash_cache, root_path, local_src_paths);
  imprint_s << hash_files(hash_cache, root_path, dependency_local_paths);
  return imprint_s.digest();
}

bool is_file_up_to_date(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::string& local_target_path,
  const std::vector<std::string>& local_src_paths,
  const command_line& command_line
) {
  auto entry = log_cache.find(local_target_path);
  if (entry == log_cache.end()) {
    return false;
  }
  auto const& record = entry->second;
  auto new_imprint = get_target_imprint(
    hash_cache,
    root_path,
    local_src_paths,
    record.dependency_local_paths,
    command_line
  );
  if (new_imprint != record.imprint) {
    return false;
  }
  auto new_hash = hash_cache.hash(root_path + "/" + local_target_path);
  return new_hash == record.hash;
}

void run_command_line(const std::string& root_path, command_line target) {
  pid_t child_pid = fork();
  if (child_pid == 0) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(target.binary_path.c_str()));
    for (auto const& arg: target.args) {
      argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    if (chdir(root_path.c_str()) != 0) {
      std::cerr << "upd: chdir() failed" << std::endl;
      _exit(1);
    }
    execvp(target.binary_path.c_str(), argv.data());
    std::cerr << "upd: execvp() failed" << std::endl;
    _exit(1);
  }
  if (child_pid < 0) {
    throw std::runtime_error("command line failed");
  }
  int status;
  if (waitpid(child_pid, &status, 0) != child_pid) {
    throw std::runtime_error("waitpid failed");
  }
  if (!WIFEXITED(status)) {
    throw std::runtime_error("process did not terminate normally");
  }
  if (WEXITSTATUS(status) != 0) {
    throw std::runtime_error("process terminated with errors");
  }
}

void update_file(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const command_line_template& param_cli,
  const std::vector<std::string>& local_src_paths,
  const std::string& local_target_path,
  const std::string& local_depfile_path,
  bool print_commands,
  directory_cache<mkdir>& dir_cache,
  const update_map& updm,
  const std::unordered_set<std::string>& local_dependency_file_paths
) {
  auto root_folder_path = root_path + '/';
  auto command_line = reify_command_line(param_cli, {
    .dependency_file = local_depfile_path,
    .input_files = local_src_paths,
    .output_files = { local_target_path }
  });
  if (is_file_up_to_date(log_cache, hash_cache, root_path, local_target_path, local_src_paths, command_line)) {
    return;
  }
  std::cout << "updating: " << local_target_path << std::endl;
  if (print_commands) {
    std::cout << "$ " << command_line << std::endl;
  }
  dir_cache.create(io::dirname_string(local_target_path));
  auto depfile_path = root_path + '/' + local_depfile_path;
  auto read_depfile_future = std::async(std::launch::async, &depfile::read, depfile_path);
  hash_cache.invalidate(root_path + '/' + local_target_path);
  std::ofstream depfile_writer(depfile_path);
  run_command_line(root_path, command_line);
  depfile_writer.close();
  std::unique_ptr<depfile::depfile_data> depfile_data = read_depfile_future.get();
  std::vector<std::string> dep_local_paths;
  std::unordered_set<std::string> local_src_path_set(local_src_paths.begin(), local_src_paths.end());
  if (depfile_data) {
    for (auto dep_path: depfile_data->dependency_paths) {
      if (dep_path.at(0) == '/') {
        if (dep_path.compare(0, root_folder_path.size(), root_folder_path) != 0) {
          // TODO: track out-of-root deps separately
          // throw std::runtime_error("depfile has a file `" + dep_path +
          //   "` out of root `" + root_folder_path + "`");
          continue;
        }
        dep_path = dep_path.substr(root_folder_path.size());
      }
      if (local_src_path_set.find(dep_path) != local_src_path_set.end()) {
        continue;
      }
      if (updm.output_files_by_path.find(dep_path) != updm.output_files_by_path.end() &&
          local_dependency_file_paths.count(dep_path) == 0) {
        throw undeclared_rule_dependency_error({
          .local_target_path = local_target_path,
          .local_dependency_path = dep_path,
        });
      }
      dep_local_paths.push_back(dep_path);
    }
  }
  auto new_imprint = get_target_imprint(
    hash_cache,
    root_path,
    local_src_paths,
    dep_local_paths,
    command_line
  );
  auto new_hash = hash_cache.hash(local_target_path);
  log_cache.record(local_target_path, {
    .dependency_local_paths = dep_local_paths,
    .hash = new_hash,
    .imprint = new_imprint
  });
}

}
