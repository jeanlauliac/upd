#include "update.h"
#include "command_line_template.h"
#include "path.h"
#include "run_command_line.h"
#include "update_worker.h"
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>

#include "inspect.h"

namespace upd {

XXH64_hash_t hash(const command_line_template_variable &target) {
  return static_cast<XXH64_hash_t>(target);
}

XXH64_hash_t hash(const command_line_template_part &part) {
  xxhash64_stream cli_hash(0);
  cli_hash << hash(part.literal_args);
  cli_hash << hash(part.variable_args);
  return cli_hash.digest();
}

XXH64_hash_t hash(const command_line_template &cli_template) {
  xxhash64_stream cli_hash(0);
  cli_hash << hash(cli_template.binary_path);
  cli_hash << hash(cli_template.parts);
  cli_hash << hash(cli_template.environment);
  return cli_hash.digest();
}

template <typename HashFile, typename Iter>
XXH64_hash_t hash_files(HashFile hash_file, Iter first, Iter last) {
  xxhash64_stream imprint_s(0);
  for (; first != last; ++first) {
    imprint_s << hash(*first) << hash_file(*first);
  }
  return imprint_s.digest();
}

template <typename HashFile, typename Cont>
XXH64_hash_t hash_files(HashFile hash_file, const Cont &container) {
  return hash_files(hash_file, container.cbegin(), container.cend());
}

typedef std::vector<std::string> string_vec;
struct imprint_dep_paths {
  const string_vec &inputs;
  const std::vector<string_vec> &dep_groups;
  const string_vec &dyn_deps;
};

XXH64_hash_t get_target_imprint(file_hash_cache &hash_cache,
                                const std::string &root_path,
                                const imprint_dep_paths &dep_paths,
                                const command_line_template &cli_template) {
  auto hash_file = [&root_path, &hash_cache](const std::string &file_path) {
    return hash_cache.hash(root_path + '/' + file_path);
  };
  xxhash64_stream imprint_s(0);
  imprint_s << hash(cli_template);
  imprint_s << hash_files(hash_file, dep_paths.inputs);
  for (auto const &group : dep_paths.dep_groups) {
    imprint_s << hash_files(hash_file, group);
  }
  imprint_s << hash_files(hash_file, dep_paths.dyn_deps);
  return imprint_s.digest();
}

bool is_file_up_to_date(update_log::cache &log_cache,
                        file_hash_cache &hash_cache,
                        const std::string &root_path,
                        const std::string &local_target_path,
                        const std::vector<std::string> &local_src_paths,
                        const std::vector<std::vector<std::string>> &dep_groups,
                        const command_line_template &cli_template) {
  auto entry = log_cache.find(local_target_path);
  if (entry == log_cache.end()) {
    return false;
  }
  auto const &record = entry->second;
  try {
    auto new_hash = hash_cache.hash(root_path + "/" + local_target_path);
    if (new_hash != record.hash) {
      throw file_changed_manually_error{local_target_path};
    }
  } catch (const std::system_error &error) {
    if (error.code() != std::errc::no_such_file_or_directory) {
      throw;
    }
    return false;
  }
  try {
    imprint_dep_paths deps_paths{local_src_paths, dep_groups,
                                 record.dependency_local_paths};
    auto new_imprint =
        get_target_imprint(hash_cache, root_path, deps_paths, cli_template);
    return new_imprint == record.imprint;
  } catch (const std::system_error &error) {
    if (error.code() != std::errc::no_such_file_or_directory) {
      throw;
    }
    return false;
  }
}

scheduled_file_update::scheduled_file_update() {}

scheduled_file_update::scheduled_file_update(
    update_job &&job_,
    std::future<std::unique_ptr<depfile::depfile_data>> &&read_depfile_future_,
    const std::string &depfile_path_, io::file_descriptor &&depfile_dummy_fd_)
    : job(std::move(job_)),
      read_depfile_future(std::move(read_depfile_future_)),
      depfile_path(depfile_path_),
      depfile_dummy_fd(std::move(depfile_dummy_fd_)) {}

scheduled_file_update::scheduled_file_update(scheduled_file_update &&other)
    : job(std::move(other.job)),
      read_depfile_future(std::move(other.read_depfile_future)),
      depfile_path(std::move(other.depfile_path)),
      depfile_dummy_fd(std::move(other.depfile_dummy_fd)) {}

scheduled_file_update &scheduled_file_update::
operator=(scheduled_file_update &&other) {
  job = std::move(other.job);
  read_depfile_future = std::move(other.read_depfile_future);
  depfile_path = std::move(other.depfile_path);
  depfile_dummy_fd = std::move(other.depfile_dummy_fd);
  return *this;
}

static std::string TEMPLATE = "/tmp/upd.XXXXXX";

scheduled_file_update
schedule_file_update(update_context &cx,
                     const command_line_template &cli_template,
                     const std::vector<std::string> &local_src_paths,
                     const std::string &local_target_path,
                     const std::vector<std::vector<std::string>> &dep_groups) {

  std::string depfile_path = io::mkdtemp_s(TEMPLATE) + "/dep";
  if (io::mkfifo(depfile_path.c_str(), 0700) != 0) io::throw_errno();

  command_line_parameters params = {
      depfile_path, local_src_paths, {local_target_path}, dep_groups};
  auto command_line =
      reify_command_line(cli_template, params, cx.root_path, io::getcwd());
  std::cout << "updating: " << local_target_path << std::endl;
  if (cx.print_commands) {
    std::cout << "$ " << command_line << std::endl;
  }
  cx.dir_cache.create(dirname(local_target_path));
  auto read_depfile_future =
      std::async(std::launch::async, &depfile::read, depfile_path);
  cx.hash_cache.invalidate(cx.root_path + '/' + local_target_path);

  int fd = io::open(depfile_path, O_WRONLY, 0600);
  io::file_descriptor depfile_dummy_fd(fd);

  return scheduled_file_update({cx.root_path, command_line},
                               std::move(read_depfile_future), depfile_path,
                               std::move(depfile_dummy_fd));
}

void finalize_scheduled_update(
    update_context &cx, scheduled_file_update &sfu,
    const command_line_template &cli_template,
    const std::vector<std::string> &local_src_paths,
    const std::vector<string_vec> &dep_groups,
    const std::string &local_target_path, const update_map &updm,
    const std::unordered_set<std::string> &order_only_dependency_file_paths) {

  sfu.depfile_dummy_fd.close();
  std::unique_ptr<depfile::depfile_data> depfile_data =
      sfu.read_depfile_future.get();
  if (io::unlink(sfu.depfile_path.c_str()) != 0) io::throw_errno();
  if (io::rmdir(dirname(sfu.depfile_path).c_str()) != 0) io::throw_errno();

  auto root_folder_path = cx.root_path + '/';

  std::vector<std::string> dep_local_paths;
  std::unordered_set<std::string> local_src_path_set(local_src_paths.begin(),
                                                     local_src_paths.end());
  if (depfile_data) {
    for (auto dep_path : depfile_data->dependency_paths) {
      dep_path = get_relative_path(cx.root_path, dep_path, io::getcwd());
      if (local_src_path_set.find(dep_path) != local_src_path_set.end()) {
        continue;
      }
      if (updm.output_files_by_path.find(dep_path) !=
              updm.output_files_by_path.end() &&
          order_only_dependency_file_paths.count(dep_path) == 0) {
        throw undeclared_rule_dependency_error({local_target_path, dep_path});
      }
      dep_local_paths.push_back(dep_path);
    }
  }
  imprint_dep_paths deps_paths{local_src_paths, dep_groups, dep_local_paths};
  auto new_imprint =
      get_target_imprint(cx.hash_cache, cx.root_path, deps_paths, cli_template);
  auto new_hash = cx.hash_cache.hash(root_folder_path + local_target_path);
  cx.log_cache.record(local_target_path,
                      {new_imprint, new_hash, dep_local_paths});
}

} // namespace upd
