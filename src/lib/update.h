#pragma once

#include "depfile.h"
#include "directory_cache.h"
#include "update_log.h"
#include "update_worker.h"
#include "xxhash64.h"
#include <future>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace upd {

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
   * If `true`, commands are printed on the output before they are executed.
   */
  bool print_commands;

  /**
   * How many update processes can be run in parallel. On multi-core processors,
   * using all cores will yield the fastest update.
   */
  size_t concurrency;
};

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

bool is_file_up_to_date(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::string& local_target_path,
  const std::vector<std::string>& local_src_paths,
  const command_line_template& cli_template
);

struct file_descriptor {
  file_descriptor(): fd_(-1) {}
  file_descriptor(int fd): fd_(fd) {}
  ~file_descriptor() { close(); }
  file_descriptor(file_descriptor& other) = delete;
  file_descriptor(file_descriptor&& other): fd_(other.fd_) {
    other.fd_ = -1;
  }
  file_descriptor& operator=(file_descriptor&) = delete;
  file_descriptor& operator=(file_descriptor&& other) {
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }
  int fd() const { return fd_; }
  void close() {
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
  };

private:
  int fd_;
};

struct scheduled_file_update {
  scheduled_file_update();
  scheduled_file_update(
    update_job&& job,
    std::future<std::unique_ptr<depfile::depfile_data>>&& read_depfile_future,
    file_descriptor&& input_fd
  );
  scheduled_file_update(scheduled_file_update&) = delete;
  scheduled_file_update(scheduled_file_update&&);
  scheduled_file_update& operator=(scheduled_file_update&&);

  update_job job;
  std::future<std::unique_ptr<depfile::depfile_data>> read_depfile_future;
  file_descriptor input_fd;
};

scheduled_file_update schedule_file_update(
  update_context& cx,
  const command_line_template& cli_template,
  const std::vector<std::string>& local_src_paths,
  const std::string& local_target_path
);

void finalize_scheduled_update(
  update_context& cx,
  scheduled_file_update& sfu,
  const command_line_template& cli_template,
  const std::vector<std::string>& local_src_paths,
  const std::string& local_target_path,
  const update_map& updm,
  const std::unordered_set<std::string>& local_dependency_file_paths
);

}
