#include "io.h"
#include "utils.h"
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>

namespace upd {
namespace io {

static void throw_errno(int value) {
  throw std::system_error(value, std::generic_category());
}

static int set_errno(int value) {
  errno = value;
  return -1;
}

template <typename Return> static Return set_errno(int value, Return retval) {
  errno = value;
  return retval;
}

std::string getcwd() { return "/home/tests"; }

bool is_regular_file(const std::string &) { return true; }

std::string mkdtemp(const std::string &) { return "/tmp/foo"; }

void mkfifo(const std::string &, mode_t) {}

enum class node_type {
  regular,
  directory,
  pts,
};

void delete_shared_fd(int *fdp) {
  ::close(*fdp);
  delete fdp;
}

struct file_node {
  node_type type;
  std::unordered_map<std::string, file_node> ents;
  std::vector<char> buf;
  std::shared_ptr<int> pts_real_pipe_fd;
};

file_node root_dir = {
    node_type::directory,
    {},
    {},
    nullptr,
};

enum class fd_type {
  file,
  pipe,
};

struct fd_data {
  fd_type type;
  file_node *node;
  size_t position;
  std::shared_ptr<int> real_pipe_fd;
};

typedef std::unordered_map<size_t, fd_data> fds_t;
fds_t fds;

struct resolution_t {
  std::vector<file_node *> node_path;
  file_node *node;
  std::string name;
};

int resolve(resolution_t &rs, const std::string &file_path) noexcept {
  if (file_path.empty()) return set_errno(ENOENT);
  if (file_path[0] != '/') {
    throw std::runtime_error("cannot resolve relative paths in this mock");
  }
  size_t slash_idx = 1;
  std::vector<file_node *> node_path;
  file_node *current_node = &root_dir;
  std::string ent_name;
  do {
    auto next_slash_idx = file_path.find('/', slash_idx + 1);
    if (current_node == nullptr) return set_errno(ENOENT);
    if (current_node->type != node_type::directory) return set_errno(ENOTDIR);
    ent_name =
        next_slash_idx == std::string::npos
            ? file_path.substr(slash_idx + 1)
            : file_path.substr(slash_idx + 1, next_slash_idx - slash_idx - 1);
    slash_idx = next_slash_idx;
    if (ent_name == "" || ent_name == ".") continue;
    if (ent_name == "..") {
      if (node_path.empty()) continue;
      current_node = node_path.back();
      node_path.pop_back();
      continue;
    }
    auto next_node_iter = current_node->ents.find(ent_name);
    node_path.push_back(current_node);
    if (next_node_iter == current_node->ents.end()) {
      current_node = nullptr;
      continue;
    }
    current_node = &next_node_iter->second;
  } while (slash_idx != std::string::npos);
  rs = {std::move(node_path), current_node, ent_name};
  return 0;
}

size_t alloc_fd() {
  int fd = 3;
  while (fds.find(fd) != fds.end() && fd < 1024) ++fd;
  if (fd == 1024) throw_errno(EMFILE);
  return fd;
}

struct dir_stream {
  file_node *node;
  std::string last_name;
};

std::unordered_map<DIR *, dir_stream> dir_streams;
size_t next_dir_stream_idx = 1;

DIR *opendir(const char *name) noexcept {
  resolution_t rs;
  if (resolve(rs, name)) return nullptr;
  if (rs.node == nullptr) return set_errno(ENOENT, nullptr);
  if (rs.node->type != node_type::directory) return set_errno(ENOTDIR, nullptr);
  auto handle = reinterpret_cast<DIR *>(next_dir_stream_idx++);
  dir_streams.emplace(handle, dir_stream{rs.node, ""});
  return handle;
}

dirent global_dirent;

struct dirent *readdir(DIR *dirp) noexcept {
  auto dir_iter = dir_streams.find(dirp);
  if (dir_iter == dir_streams.end()) return set_errno(EINVAL, nullptr);
  auto &ds = dir_iter->second;
  auto iter = ds.last_name.empty() ? ds.node->ents.begin()
                                   : ++ds.node->ents.find(ds.last_name);
  if (iter == ds.node->ents.end()) return nullptr;
  global_dirent.d_reclen = sizeof(dirent);
  global_dirent.d_type = DT_UNKNOWN;
  ds.last_name = iter->first;
  if (ds.last_name.size() >= sizeof(global_dirent.d_name))
    return set_errno(EIO, nullptr);
  strcpy(global_dirent.d_name, ds.last_name.c_str());
  return &global_dirent;
}

int closedir(DIR *dirp) noexcept {
  auto iter = dir_streams.find(dirp);
  if (iter == dir_streams.end()) return set_errno(EINVAL);
  dir_streams.erase(iter);
  return 0;
}

int mkdir(const char *path, mode_t) noexcept {
  resolution_t rs;
  if (resolve(rs, path)) return -1;
  if (rs.node != nullptr) return set_errno(EEXIST);
  rs.node_path.back()->ents.emplace(
      rs.name, file_node{node_type::directory, {}, {}, nullptr});
  return 0;
}

int open(const std::string &file_path, int flags, mode_t) {
  resolution_t rs;
  if (resolve(rs, file_path)) throw_errno();
  auto node = rs.node;
  if (node == nullptr) {
    if ((flags & O_CREAT) == 0) throw_errno(ENOENT);
    auto result = rs.node_path.back()->ents.emplace(
        rs.name, file_node{node_type::regular, {}, {}, nullptr});
    node = &result.first->second;
  } else if (node->type == node_type::pts) {
    auto fd = alloc_fd();
    fds[fd] = {fd_type::pipe, node, 0, node->pts_real_pipe_fd};
    // FIXME: this prevent reusing the same ptsname() twice, that does not
    // fit the way it really works.
    node->pts_real_pipe_fd.reset();
    return fd;
  }
  auto fd = alloc_fd();
  fds[fd] = {fd_type::file, node, 0, nullptr};
  return fd;
}

size_t write(fd_data &desc, const void *buf, size_t size) {
  if (desc.type == fd_type::pipe) {
    size_t bytes = ::write(*desc.real_pipe_fd, buf, size);
    if (bytes != size) throw_errno();
    return bytes;
  }
  auto &file_buf = desc.node->buf;
  auto new_size = desc.position + size;
  if (file_buf.size() < new_size) {
    file_buf.resize(new_size);
  }
  std::memcpy(file_buf.data() + desc.position, buf, size);
  desc.position += size;
  return size;
}

size_t write(int fd, const void *buf, size_t size) {
  return write(fds.at(fd), buf, size);
}

ssize_t read(int fd, void *buf, size_t size) {
  auto &desc = fds.at(fd);
  if (desc.type == fd_type::pipe) {
    ssize_t bytes = ::read(*desc.real_pipe_fd, buf, size);
    if (bytes < 0) throw_errno();
    return bytes;
  }
  auto &file_buf = desc.node->buf;
  if (desc.position + size > file_buf.size()) {
    size = file_buf.size() - desc.position;
  }
  std::memcpy(buf, file_buf.data() + desc.position, size);
  desc.position += size;
  return size;
}

void close(int fd) { fds.erase(fd); }

int lstat(const char *path, struct ::stat *buf) noexcept {
  resolution_t rs;
  if (resolve(rs, path)) return -1;
  if (rs.node == nullptr) set_errno(ENOENT);
  buf->st_dev = 999;
  buf->st_ino = 777;
  buf->st_mode = 0666;
  if (rs.node->type == node_type::regular)
    buf->st_mode |= S_IFREG;
  else if (rs.node->type == node_type::directory)
    buf->st_mode |= S_IFDIR;
  else if (rs.node->type == node_type::pts)
    buf->st_mode |= S_IFIFO;
  buf->st_nlink = 1;
  buf->st_uid = 1;
  buf->st_gid = 2;
  buf->st_size = rs.node->buf.size();
  return 0;
}

int posix_openpt(int) {
  std::array<int, 2> real_pipe_fds;
  if (::pipe(real_pipe_fds.data()) != 0) throw_errno(errno);
  auto master_pt_fd = alloc_fd();
  fds[master_pt_fd] = {
      fd_type::pipe, nullptr, 0,
      std::shared_ptr<int>(new int(real_pipe_fds[0]), delete_shared_fd)};
  try {
    mkdir("/pseudoterminal", 0);
  } catch (std::system_error error) {
    if (error.code() != std::errc::file_exists) throw;
  }
  auto pts_file_path = "/pseudoterminal/" + std::to_string(master_pt_fd);
  resolution_t rs;
  if (resolve(rs, pts_file_path)) throw_errno(errno);
  rs.node_path.back()->ents.emplace(
      rs.name, file_node{node_type::pts,
                         {},
                         {},
                         std::shared_ptr<int>(new int(real_pipe_fds[1]),
                                              delete_shared_fd)});
  return master_pt_fd;
}

void grantpt(int) {}
void unlockpt(int) {}

std::string ptsname(int fd) { return "/pseudoterminal/" + std::to_string(fd); }

void pipe(int pipefd[2]) {
  std::array<int, 2> real_pipe_fds;
  if (::pipe(real_pipe_fds.data()) != 0) throw_errno(errno);
  auto read_fd = pipefd[0] = alloc_fd();
  fds[read_fd] = {
      fd_type::pipe, nullptr, 0,
      std::shared_ptr<int>(new int(real_pipe_fds[0]), delete_shared_fd)};
  auto write_fd = pipefd[1] = alloc_fd();
  fds[write_fd] = {
      fd_type::pipe, nullptr, 0,
      std::shared_ptr<int>(new int(real_pipe_fds[1]), delete_shared_fd)};
}

int isatty(int fd) {
  auto &desc = fds[fd];
  return desc.node->type == node_type::pts;
}

enum class action_type { dup2, close };

struct file_action {
  action_type type;
  int fd;
  int new_fd;
};

struct posix_spawn_file_actions_mock {
  std::vector<file_action> acts;
};

std::unordered_map<const posix_spawn_file_actions_t *,
                   posix_spawn_file_actions_mock>
    file_action_entries;

void posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions,
                                       int fd) {
  auto actions_iter = file_action_entries.find(actions);
  if (actions_iter == file_action_entries.end()) throw_errno(EINVAL);
  actions_iter->second.acts.push_back({action_type::close, fd, -1});
}

void posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions,
                                      int fd, int new_fd) {
  auto actions_iter = file_action_entries.find(actions);
  if (actions_iter == file_action_entries.end()) throw_errno(EINVAL);
  actions_iter->second.acts.push_back({action_type::dup2, fd, new_fd});
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
  auto actions_iter = file_action_entries.find(file_actions);
  if (actions_iter == file_action_entries.end()) throw_errno(EINVAL);
  auto reg_bin = reg_bins.find(binary_path);
  if (reg_bin == reg_bins.end()) throw_errno(ENOENT);
  *pid = next_pid++;
  mock::spawn_records.push_back(mock::spawn_record{
      binary_path, ntsa_to_vector(args), ntsa_to_vector(env)});
  fds_t proc_fds = fds;
  for (auto const &act : actions_iter->second.acts) {
    switch (act.type) {
    case action_type::dup2: {
      auto iter = proc_fds.find(act.fd);
      if (iter == proc_fds.end()) throw_errno(EINVAL);
      proc_fds[act.new_fd] = iter->second;
      break;
    }
    case action_type::close: {
      proc_fds.erase(act.fd);
      break;
    }
    }
  }
  auto const &stdout = reg_bin->second.stdout;
  write(proc_fds[STDOUT_FILENO], stdout.c_str(), stdout.size());
  auto const &stderr = reg_bin->second.stderr;
  write(proc_fds[STDERR_FILENO], stderr.c_str(), stderr.size());
}

pid_t waitpid(pid_t pid, int *, int) { return pid; }

namespace mock {

void reset() {
  root_dir = {
      node_type::directory,
      {},
      {},
      nullptr,
  };
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
