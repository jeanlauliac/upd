#include "command_line_template.h"
#include "command_line_runner.h"
#include "depfile.h"
#include "path.h"
#include "update.h"
#include <stdexcept>
#include <sys/wait.h>

namespace upd {

XXH64_hash_t hash(const command_line_template_variable& target) {
  return static_cast<XXH64_hash_t>(target);
}

XXH64_hash_t hash(const command_line_template_part& part) {
  xxhash64_stream cli_hash(0);
  cli_hash << hash(part.literal_args);
  cli_hash << hash(part.variable_args);
  return cli_hash.digest();
}

XXH64_hash_t hash(const command_line_template& cli_template) {
  xxhash64_stream cli_hash(0);
  cli_hash << hash(cli_template.binary_path);
  cli_hash << hash(cli_template.parts);
  return cli_hash.digest();
}

XXH64_hash_t hash_files(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_paths
) {
  xxhash64_stream imprint_s(0);
  for (auto const& local_path: local_paths) {
    imprint_s << hash(local_path);
    imprint_s << hash_cache.hash(root_path + '/' + local_path);
  }
  return imprint_s.digest();
}

XXH64_hash_t get_target_imprint(
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const std::vector<std::string>& local_src_paths,
  std::vector<std::string> dependency_local_paths,
  const command_line_template& cli_template
) {
  xxhash64_stream imprint_s(0);
  imprint_s << hash(cli_template);
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
  const command_line_template& cli_template
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
    cli_template
  );
  if (new_imprint != record.imprint) {
    return false;
  }
  auto new_hash = hash_cache.hash(root_path + "/" + local_target_path);
  return new_hash == record.hash;
}

std::string get_fd_path(int fd) {
  std::ostringstream oss;
  oss << "/dev/fd/" << fd;
  return oss.str();
}

struct file_descriptor {
  file_descriptor(int fd): fd_(fd) {}
  ~file_descriptor() { close(); }
  file_descriptor(file_descriptor& other) = delete;
  file_descriptor(file_descriptor&& other) = delete;
  int fd() const { return fd_; }
  void close() {
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
  };

private:
  int fd_;
};

struct update_job {
  const std::string root_path;
  command_line target;
  int depfile_fds[2];
};

std::mutex receive_job_mutex;
std::condition_variable receive_job_cv;
bool shutdown_requested = false;
std::unique_ptr<update_job> job;

bool finished = false;
std::mutex finish_job_mutex;
std::condition_variable finish_job_cv;

void start_update_thread() {
  std::unique_lock<std::mutex> lk(receive_job_mutex);
  while (!shutdown_requested) {
    if (!job) {
      receive_job_cv.wait(lk);
      continue;
    }
    command_line_runner::run(job->root_path, job->target, job->depfile_fds);
    job.reset();
    std::unique_lock<std::mutex> fj(finish_job_mutex);
    finished = true;
    fj.unlock();
    finish_job_cv.notify_one();
  }
}

void update_file(
  update_context& cx,
  const command_line_template& cli_template,
  const std::vector<std::string>& local_src_paths,
  const std::string& local_target_path,
  const update_map& updm,
  const std::unordered_set<std::string>& local_dependency_file_paths
) {
  const auto& root_path = cx.root_path;
  if (is_file_up_to_date(cx.log_cache, cx.hash_cache, root_path, local_target_path, local_src_paths, cli_template)) {
    return;
  }
  auto root_folder_path = root_path + '/';

  int depfile_fds[2];
  if (pipe(depfile_fds) != 0) {
    throw new std::runtime_error("pipe() failed");
  }
  file_descriptor input_fd(depfile_fds[0]);

  auto command_line = reify_command_line(cli_template, {
    .dependency_file = get_fd_path(depfile_fds[1]),
    .input_files = local_src_paths,
    .output_files = { local_target_path }
  });
  std::cout << "updating: " << local_target_path << std::endl;
  if (cx.print_commands) {
    std::cout << "$ " << command_line << std::endl;
  }
  cx.dir_cache.create(io::dirname_string(local_target_path));
  auto read_depfile_future = std::async(std::launch::async, &depfile::read, input_fd.fd());
  cx.hash_cache.invalidate(root_path + '/' + local_target_path);



  std::thread update_thread(start_update_thread);
  {
    std::lock_guard<std::mutex> lg(receive_job_mutex);
    shutdown_requested = false;
    job = std::make_unique<update_job>(update_job {
      .depfile_fds = { depfile_fds[0], depfile_fds[1] },
      .root_path = root_path,
      .target = command_line,
    });
  }
  receive_job_cv.notify_one();
  {
    std::unique_lock<std::mutex> lk(finish_job_mutex);
    if (!finished) finish_job_cv.wait(lk, []{ return finished; });
    finished = false;
  }
  {
    std::lock_guard<std::mutex> lg(receive_job_mutex);
    shutdown_requested = true;
  }
  receive_job_cv.notify_one();
  update_thread.join();



  std::unique_ptr<depfile::depfile_data> depfile_data = read_depfile_future.get();
  input_fd.close();

  std::vector<std::string> dep_local_paths;
  std::unordered_set<std::string> local_src_path_set(local_src_paths.begin(), local_src_paths.end());
  if (depfile_data) {
    for (auto dep_path: depfile_data->dependency_paths) {
      dep_path = normalize_path(dep_path);
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
    cx.hash_cache,
    root_path,
    local_src_paths,
    dep_local_paths,
    cli_template
  );
  auto new_hash = cx.hash_cache.hash(local_target_path);
  cx.log_cache.record(local_target_path, {
    .dependency_local_paths = dep_local_paths,
    .hash = new_hash,
    .imprint = new_imprint
  });
}

}
