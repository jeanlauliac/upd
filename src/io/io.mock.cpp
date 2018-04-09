#include "io.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unordered_map>

namespace upd {
namespace io {

static void throw_errno(int value) {
  throw std::system_error(value, std::generic_category());
}

std::string getcwd() { return "/home/tests"; }

bool is_regular_file(const std::string &) { return true; }

dir::dir(const std::string &) : ptr_(nullptr) {}

dir::dir() : ptr_(nullptr) {}

dir::~dir() {}

void dir::open(const std::string &) {}

void dir::close() {}

dir_files_reader::dir_files_reader(const std::string &) {}
dir_files_reader::dir_files_reader() {}

struct dirent *dir_files_reader::next() {
  return nullptr;
}

void dir_files_reader::open(const std::string &) {}

void dir_files_reader::close() {}

std::string mkdtemp(const std::string &) { return "/tmp/foo"; }

void mkfifo(const std::string &, mode_t) {}

enum class file_type {
  regular,
  pts,
};

struct file_data {
  file_type type;
  std::vector<char> buf;
};

struct fd_data {
  std::string file_path;
  size_t position;
};

std::unordered_map<std::string, file_data> files;
std::unordered_map<size_t, fd_data> fds;

size_t alloc_fd() {
  int fd = 3;
  while (fds.find(fd) != fds.end() && fd < 1024) ++fd;
  if (fd == 1024) throw_errno(EMFILE);
  return fd;
}

int open(const std::string &file_path, int flags, mode_t) {
  if (files.find(file_path) == files.end()) {
    if ((flags & O_CREAT) == 0) throw_errno(ENOENT);
    files[file_path] = {file_type::regular, {}};
  }
  auto fd = alloc_fd();
  fds[fd] = {file_path, 0};
  return fd;
}

size_t write(int fd, const void *buf, size_t size) {
  auto &desc = fds[fd];
  auto &file_buf = files[desc.file_path].buf;
  auto new_size = desc.position + size;
  if (file_buf.size() < new_size) {
    file_buf.resize(new_size);
  }
  std::memcpy(file_buf.data() + desc.position, buf, size);
  desc.position += size;
  return size;
}

ssize_t read(int fd, void *buf, size_t size) {
  auto &desc = fds[fd];
  auto &file_buf = files[desc.file_path].buf;
  if (desc.position + size > file_buf.size()) {
    size = file_buf.size() - desc.position;
  }
  std::memcpy(buf, file_buf.data() + desc.position, size);
  desc.position += size;
  return size;
}

void close(int fd) { fds.erase(fd); }

int posix_openpt(int) {
  auto fd = alloc_fd();
  fds[fd] = {"", 0};
  return fd;
}

void grantpt(int) {}
void unlockpt(int) {}

std::string ptsname(int fd) {
  auto name = "/pseudoterminal/" + std::to_string(fd);
  files[name] = {file_type::pts, {}};
  return name;
}

void pipe(int pipefd[2]) {
  pipefd[0] = alloc_fd();
  pipefd[1] = alloc_fd();
}

int isatty(int fd) {
  auto &desc = fds[fd];
  return files[desc.file_path].type == file_type::pts;
}

struct posix_spawn_file_actions_mock {};

std::unordered_map<const posix_spawn_file_actions_t *,
                   posix_spawn_file_actions_mock>
    file_action_entries;

void posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions,
                                       int) {
  if (file_action_entries.find(actions) == file_action_entries.end())
    throw_errno(EINVAL);
}

void posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions, int,
                                      int) {
  if (file_action_entries.find(actions) == file_action_entries.end())
    throw_errno(EINVAL);
}

void posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *actions, int,
                                      const char *, int, mode_t) {
  if (file_action_entries.find(actions) == file_action_entries.end())
    throw_errno(EINVAL);
}

void posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *actions) {
  if (file_action_entries.find(actions) == file_action_entries.end())
    throw_errno(EINVAL);
  file_action_entries.erase(actions);
}

void posix_spawn_file_actions_init(posix_spawn_file_actions_t *actions) {
  if (file_action_entries.find(actions) != file_action_entries.end())
    throw_errno(EINVAL);
  file_action_entries[actions] = {};
}

struct registered_binary {
  std::string stdout;
  std::string stderr;
};

static int next_pid = (1 << 16) + 1;
static std::unordered_map<std::string, registered_binary> reg_bins;

namespace mock {
std::vector<spawn_record> spawn_records;
}

std::vector<std::string> ntsa_to_vector(char *const strs[]) {
  std::vector<std::string> result;
  while (*strs != nullptr) {
    result.push_back(*strs);
    ++strs;
  }
  return result;
}

void posix_spawn(pid_t *pid, const char *binary_path,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *, char *const args[],
                 char *const env[]) {
  if (file_action_entries.find(file_actions) == file_action_entries.end())
    throw_errno(EINVAL);
  auto reg_bin = reg_bins.find(binary_path);
  if (reg_bin == reg_bins.end()) throw_errno(ENOENT);
  *pid = next_pid++;
  mock::spawn_records.push_back(mock::spawn_record{
      binary_path, ntsa_to_vector(args), ntsa_to_vector(env)});
}

pid_t waitpid(pid_t pid, int *, int) { return pid; }

namespace mock {

void reset() {
  files.clear();
  fds.clear();
  file_action_entries.clear();
}

void register_binary(const std::string &binary_path, const std::string &stdout,
                     const std::string &stderr) {
  reg_bins[binary_path] = {stdout, stderr};
}

} // namespace mock

} // namespace io
} // namespace upd
