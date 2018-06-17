#include "io.h"
#include "utils.h"
#include <condition_variable>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <random>
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

std::minstd_rand0 rand;
std::uniform_int_distribution<char> char_dis('A', 'Z');
std::uniform_int_distribution<int> bool_dis(0, 1);

char *mkdtemp(char *tpl) noexcept {
  auto len = std::strlen(tpl);
  if (len < 6) return set_errno(EINVAL, nullptr);
  for (size_t i = len - 6; i < len; ++i) {
    if (tpl[i] != 'X') return set_errno(EINVAL, nullptr);
  }
  int retval;
  do {
    for (size_t i = len - 6; i < len; ++i) {
      tpl[i] = char_dis(rand);
      if (bool_dis(rand) > 0) {
        tpl[i] += 'a' - 'A';
      }
    }
    retval = io::mkdir(tpl, 0700);
  } while (retval == 0 && errno == EEXIST);
  if (retval != 0) return nullptr;
  return tpl;
}

enum class node_type {
  regular,
  directory,
  fifo,
  pts,
};

struct real_fd {
  real_fd(int fd) : _fd(fd) {}
  ~real_fd() {
    if (_fd >= 0) ::close(_fd);
  }
  int get() { return _fd; }

private:
  int _fd;
};

struct file_node {
  ~file_node() {}

  node_type type;
  std::unordered_map<std::string, std::shared_ptr<file_node>> ents;
  std::vector<char> buf;
  std::shared_ptr<real_fd> pts_real_pipe_fd;
  size_t readers_count;
  size_t writers_count;
};

std::shared_ptr<file_node> root_dir(new file_node{
    node_type::directory,
    {},
    {},
    nullptr,
    0,
    0,
});

std::mutex gm;
std::condition_variable fifo_cv;

enum class fd_type {
  file,
  pipe,
};

struct fd_data {
  fd_type type;
  std::shared_ptr<file_node> node;
  size_t position;
  std::shared_ptr<real_fd> real_pipe_fd;
  bool readable;
  bool writable;
};

typedef std::unordered_map<size_t, fd_data> fds_t;
fds_t fds;

struct resolution_t {
  std::vector<std::shared_ptr<file_node>> node_path;
  std::shared_ptr<file_node> node;
  std::string name;
};

int resolve(resolution_t &rs, const std::string &file_path) noexcept {
  if (file_path.empty()) return set_errno(ENOENT);
  if (file_path[0] != '/') {
    throw std::runtime_error("cannot resolve relative paths in this mock");
  }
  size_t slash_idx = 0;
  std::vector<std::shared_ptr<file_node>> node_path;
  std::shared_ptr<file_node> current_node = root_dir;
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
    current_node = next_node_iter->second;
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
  std::shared_ptr<file_node> node;
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
  dir_streams.emplace(handle, dir_stream{std::move(rs.node), ""});
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
      rs.name, new file_node{node_type::directory, {}, {}, nullptr, 0, 0});
  return 0;
}

int rmdir(const char *path) noexcept {
  resolution_t rs;
  if (resolve(rs, path)) return -1;
  if (rs.node == nullptr) return set_errno(ENOENT);
  if (rs.node->type != node_type::directory) return set_errno(ENOTDIR);
  if (rs.node->ents.size() != 0) return set_errno(EEXIST);
  if (rs.node_path.size() == 0) return set_errno(EPERM);
  auto &dir_node = rs.node_path.back();
  dir_node->ents.erase(rs.name);
  return 0;
}

int open(const std::string &file_path, int flags, mode_t) {
  std::unique_lock<std::mutex> lock(gm);
  resolution_t rs;
  if (resolve(rs, file_path)) throw_errno();
  auto node = rs.node;
  if (node == nullptr) {
    if ((flags & O_CREAT) == 0) throw_errno(ENOENT);
    auto result = rs.node_path.back()->ents.emplace(
        rs.name, new file_node{node_type::regular, {}, {}, nullptr, 0, 0});
    node = result.first->second;
  } else if (node->type == node_type::pts) {
    auto fd = alloc_fd();
    fds[fd] = {fd_type::pipe, node, 0, node->pts_real_pipe_fd, true, true};
    // FIXME: this prevent reusing the same ptsname() twice, that does not
    // fit the way it really works.
    node->pts_real_pipe_fd.reset();
    return fd;
  }
  if (node->type == node_type::fifo) {
    if ((flags & O_RDWR) > 0) {
      throw_errno(EINVAL);
    }
    if ((flags & O_WRONLY) > 0) {
      ++node->writers_count;
      fifo_cv.notify_all();
      while (node->readers_count == 0) fifo_cv.wait(lock);
    } else {
      ++node->readers_count;
      fifo_cv.notify_all();
      while (node->writers_count == 0 && node->buf.empty()) fifo_cv.wait(lock);
    }
  }
  auto fd = alloc_fd();
  fds[fd] = {fd_type::file,
             node,
             0,
             nullptr,
             (flags & O_WRONLY) == 0,
             (flags & O_WRONLY) > 0 || (flags & O_RDWR) > 0};
  return fd;
}

int mkfifo(const char *path, mode_t) noexcept {
  resolution_t rs;
  if (resolve(rs, path)) return -1;
  if (rs.node != nullptr) return set_errno(EEXIST);
  rs.node_path.back()->ents.emplace(
      rs.name, new file_node{node_type::fifo, {}, {}, nullptr, 0, 0});
  return 0;
}

size_t write(fd_data &desc, const void *buf, size_t size) {
  std::unique_lock<std::mutex> lock(gm);
  if (desc.type == fd_type::pipe) {
    auto real_fd = desc.real_pipe_fd->get();
    // FIXME: this is unsafe in theory. If an exception was thrown, it does
    // not get locked again, though local objects' destructor might access
    // shared data.
    lock.unlock();
    size_t bytes = ::write(real_fd, buf, size);
    lock.lock();
    if (bytes != size) throw_errno();
    return bytes;
  }
  auto &file_buf = desc.node->buf;
  if (desc.node->type == node_type::fifo) {
    while (file_buf.size() > 1024) {
      fifo_cv.wait(lock);
    }
    desc.position = file_buf.size();
  }
  auto new_size = desc.position + size;
  if (file_buf.size() < new_size) {
    file_buf.resize(new_size);
  }
  std::memcpy(file_buf.data() + desc.position, buf, size);
  desc.position += size;
  if (desc.node->type == node_type::fifo) {
    lock.unlock();
    fifo_cv.notify_all();
  }
  return size;
}

size_t write(int fd, const void *buf, size_t size) {
  return write(fds.at(fd), buf, size);
}

ssize_t read(int fd, void *buf, size_t size) {
  std::unique_lock<std::mutex> lock(gm);
  auto &desc = fds.at(fd);
  if (desc.type == fd_type::pipe) {
    auto real_fd = desc.real_pipe_fd->get();
    lock.unlock();
    ssize_t bytes = ::read(real_fd, buf, size);
    lock.lock();
    if (bytes < 0) throw_errno();
    return bytes;
  }
  auto &file_buf = desc.node->buf;
  if (desc.node->type == node_type::fifo) {
    while (file_buf.size() == 0 && desc.node->writers_count > 0) {
      fifo_cv.wait(lock);
    }
    if (file_buf.size() == 0) {
      return 0;
    }
    desc.position = 0;
  }
  if (desc.position + size > file_buf.size()) {
    size = file_buf.size() - desc.position;
  }
  std::memcpy(buf, file_buf.data() + desc.position, size);
  desc.position += size;
  if (desc.node->type == node_type::fifo) {
    file_buf.erase(file_buf.begin(), file_buf.begin() + size);
    lock.unlock();
    fifo_cv.notify_all();
  }
  return size;
}

void close(int fd) {
  std::unique_lock<std::mutex> lock(gm);
  auto &desc = fds.at(fd);
  if (desc.type == fd_type::file && desc.node->type == node_type::fifo) {
    if (desc.readable) desc.node->readers_count--;
    if (desc.writable) desc.node->writers_count--;
    fifo_cv.notify_all();
  }
  fds.erase(fd);
}

int rename(const char *old_path, const char *new_path) noexcept {
  std::unique_lock<std::mutex> lock(gm);
  resolution_t old_rs, new_rs;
  if (resolve(old_rs, old_path)) return -1;
  if (old_rs.node == nullptr) return set_errno(ENOENT);
  if (resolve(new_rs, new_path)) return -1;
  if (new_rs.node != nullptr && new_rs.node->type == node_type::directory)
    return set_errno(EEXIST);
  auto &old_dir_node = old_rs.node_path.back();
  old_dir_node->ents.erase(old_rs.name);
  auto &new_dir_node = new_rs.node_path.back();
  new_dir_node->ents[new_rs.name] = std::move(old_rs.node);
  return 0;
}

int lstat(const char *path, struct ::stat *buf) noexcept {
  std::unique_lock<std::mutex> lock(gm);
  resolution_t rs;
  if (resolve(rs, path)) return -1;
  if (rs.node == nullptr) return set_errno(ENOENT);
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

int unlink(const char *ent_path) noexcept {
  resolution_t rs;
  if (resolve(rs, ent_path)) return -1;
  if (rs.node == nullptr) return set_errno(ENOENT);
  if (rs.node->type == node_type::directory) return set_errno(EISDIR);
  if (rs.node_path.size() == 0) return set_errno(EPERM);
  auto &dir_node = rs.node_path.back();
  dir_node->ents.erase(rs.name);
  return 0;
}

int posix_openpt(int) {
  std::array<int, 2> real_pipe_fds;
  if (::pipe(real_pipe_fds.data()) != 0) throw_errno(errno);
  auto master_pt_fd = alloc_fd();
  fds[master_pt_fd] = {
      fd_type::pipe, nullptr, 0, std::make_shared<real_fd>(real_pipe_fds[0]),
      true,          true};
  try {
    mkdir("/pseudoterminal", 0);
  } catch (std::system_error error) {
    if (error.code() != std::errc::file_exists) throw;
  }
  auto pts_file_path = "/pseudoterminal/" + std::to_string(master_pt_fd);
  resolution_t rs;
  if (resolve(rs, pts_file_path)) throw_errno(errno);
  rs.node_path.back()->ents.emplace(
      rs.name, new file_node{node_type::pts,
                             {},
                             {},
                             std::make_shared<real_fd>(real_pipe_fds[1]),
                             0,
                             0});
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
      fd_type::pipe, nullptr, 0, std::make_shared<real_fd>(real_pipe_fds[0]),
      true,          false};
  auto write_fd = pipefd[1] = alloc_fd();
  fds[write_fd] = {
      fd_type::pipe, nullptr, 0, std::make_shared<real_fd>(real_pipe_fds[1]),
      false,         true};
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
  mock::binary_fn fn;
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
  // FIXME: use resolve() instead.
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
  if (reg_bin->second.fn) {
    reg_bin->second.fn(args);
  }
  auto const &stdout = reg_bin->second.stdout;
  write(proc_fds[STDOUT_FILENO], stdout.c_str(), stdout.size());
  auto const &stderr = reg_bin->second.stderr;
  write(proc_fds[STDERR_FILENO], stderr.c_str(), stderr.size());
}

pid_t waitpid(pid_t pid, int *stat_loc, int) {
  if (stat_loc != nullptr) {
    *stat_loc = 0;
  }
  return pid;
}

namespace mock {

void reset() {
  root_dir.reset(new file_node{
      node_type::directory,
      {},
      {},
      nullptr,
      0,
      0,
  });
  fds.clear();
  file_action_entries.clear();
  reg_bins.clear();
  mock::spawn_records.clear();
}

void register_binary(const std::string &binary_path, std::string stdout,
                     std::string stderr, binary_fn fn) {
  reg_bins[binary_path] = {std::move(stdout), std::move(stderr), std::move(fn)};
}

} // namespace mock

} // namespace io
} // namespace upd
