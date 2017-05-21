#pragma once

#include "directory_cache.h"
#include "update_log.h"
#include "xxhash64.h"
#include <future>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace upd {

struct output_file {
  size_t command_line_ix;
  std::vector<std::string> local_input_file_paths;
  std::unordered_set<std::string> local_dependency_file_paths;
};

typedef std::unordered_map<std::string, output_file> output_files_by_path_t;

struct update_map {
  output_files_by_path_t output_files_by_path;
};

struct undeclared_rule_dependency_error {
  std::string local_target_path;
  std::string local_dependency_path;
};

XXH64_hash_t hash_command_line(const command_line& command_line);

XXH64_hash_t hash_files(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_paths
);

XXH64_hash_t get_target_imprint(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_src_paths,
  std::vector<std::string> dependency_local_paths,
  const command_line& command_line
);

bool is_file_up_to_date(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::string& local_target_path,
  const std::vector<std::string>& local_src_paths,
  const command_line& command_line
);

void run_command_line(const std::string& root_path, command_line target);

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
);

}
