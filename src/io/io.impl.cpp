#include "../path.h"
#include "io.h"
#include <cstring>
#include <fcntl.h>
#include <libgen.h>
#include <stdexcept>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace upd {
namespace io {

void throw_errno() { throw std::system_error(errno, std::generic_category()); }

std::string getcwd() {
  char temp[MAXPATHLEN];
  if (::getcwd(temp, MAXPATHLEN) == nullptr) throw_errno();
  return temp;
}

bool is_regular_file(const std::string &path) {
  struct stat data;
  auto stat_ret = stat(path.c_str(), &data);
  if (stat_ret != 0) {
    if (errno == ENOENT) {
      return false;
    }
    throw std::runtime_error("`stat` function returned unhanled error");
  }
  return S_ISREG(data.st_mode) != 0;
}

dir::dir(const std::string &path) : ptr_(opendir(path.c_str())) {
  if (ptr_ == nullptr) throw std::runtime_error("opendir() failed");
}

dir::dir() : ptr_(nullptr) {}

dir::~dir() {
  if (ptr_ != nullptr) closedir(ptr_);
}

void dir::open(const std::string &path) {
  if (ptr_ != nullptr) closedir(ptr_);
  ptr_ = opendir(path.c_str());
  if (ptr_ == nullptr) throw std::runtime_error("opendir() failed");
}

void dir::close() {
  closedir(ptr_);
  ptr_ = nullptr;
}

dir_files_reader::dir_files_reader(const std::string &path) : target_(path) {}
dir_files_reader::dir_files_reader() {}

struct dirent *dir_files_reader::next() {
  if (target_.ptr() == nullptr) throw std::runtime_error("no dir is open");
  return readdir(target_.ptr());
}

void dir_files_reader::open(const std::string &path) { target_.open(path); }

void dir_files_reader::close() { target_.close(); }

std::string mkdtemp(const std::string &template_path) {
  std::vector<char> tpl(template_path.size() + 1);
  strcpy(tpl.data(), template_path.c_str());
  if (::mkdtemp(tpl.data()) == nullptr) throw_errno();
  return tpl.data();
}

void mkfifo(const std::string &file_path, mode_t mode) {
  if (::mkfifo(file_path.c_str(), mode) != 0) throw_errno();
}

void mkdir(const std::string &dir_path, mode_t mode) {
  if (::mkdir(dir_path.c_str(), mode) != 0) throw_errno();
}

int open(const std::string &file_path, int flags, mode_t mode) {
  int fd = ::open(file_path.c_str(), flags, mode);
  if (fd < 0) {
    throw std::system_error(errno, std::generic_category());
  }
  return fd;
}

size_t write(int fd, const void *buf, size_t count) {
  ssize_t bytes_written = ::write(fd, buf, count);
  if (bytes_written < 0) throw_errno();
  return bytes_written;
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t bytes_read = ::read(fd, buf, count);
  if (bytes_read < 0) throw_errno();
  return bytes_read;
}

void close(int fd) {
  if (::close(fd) != 0) throw_errno();
}

int posix_openpt(int oflag) {
  int fd = ::posix_openpt(oflag);
  if (fd < 0) throw_errno();
  return fd;
}

void grantpt(int fd) {
  if (::grantpt(fd) != 0) throw_errno();
}

void unlockpt(int fd) {
  if (::unlockpt(fd) != 0) throw_errno();
}

std::string ptsname(int fd) {
  char *c = ::ptsname(fd);
  if (c == nullptr) throw_errno();
  return c;
}

void pipe(int pipefd[2]) {
  if (::pipe(pipefd) != 0) throw_errno();
}

int isatty(int fd) { return ::isatty(fd); }

void posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions,
                                       int fd) {
  if (::posix_spawn_file_actions_addclose(actions, fd) != 0) throw_errno();
}

void posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions,
                                      int fd, int new_fd) {
  if (::posix_spawn_file_actions_adddup2(actions, fd, new_fd) != 0)
    throw_errno();
}

void posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *actions,
                                      int fd, const char *file_path, int oflag,
                                      mode_t mode) {
  if (::posix_spawn_file_actions_addopen(actions, fd, file_path, oflag, mode) !=
      0)
    throw_errno();
}

void posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *actions) {
  if (::posix_spawn_file_actions_destroy(actions) != 0) throw_errno();
}

void posix_spawn_file_actions_init(posix_spawn_file_actions_t *actions) {
  if (::posix_spawn_file_actions_init(actions) != 0) throw_errno();
}

void posix_spawn(pid_t *pid, const char *path,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp, char *const argv[],
                 char *const envp[]) {
  if (::posix_spawn(pid, path, file_actions, attrp, argv, envp) != 0)
    throw_errno();
}

pid_t waitpid(pid_t pid, int *status, int options) {
  auto rpid = ::waitpid(pid, status, options);
  if (rpid < 0) throw_errno();
  return rpid;
}

} // namespace io
} // namespace upd
