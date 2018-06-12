#include "../path.h"
#include "io.h"
#include "utils.h"
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

DIR *opendir(const char *name) noexcept { return ::opendir(name); }
struct dirent *readdir(DIR *dirp) noexcept {
  return ::readdir(dirp);
}
int closedir(DIR *dirp) noexcept { return ::closedir(dirp); }

char *mkdtemp(char *tpl) noexcept { return ::mkdtemp(tpl); }

int mkdir(const char *path, mode_t mode) noexcept {
  return ::mkdir(path, mode);
}

int rmdir(const char *path) noexcept { return ::rmdir(path); }

int open(const std::string &file_path, int flags, mode_t mode) {
  int fd = ::open(file_path.c_str(), flags, mode);
  if (fd < 0) {
    throw std::system_error(errno, std::generic_category());
  }
  return fd;
}

int mkfifo(const char *path, mode_t mode) noexcept {
  return ::mkfifo(path, mode);
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

int rename(const char *old_path, const char *new_path) noexcept {
  return ::rename(old_path, new_path);
}

int lstat(const char *path, struct ::stat *buf) noexcept {
  return ::lstat(path, buf);
}

int unlink(const char *pathname) noexcept { return ::unlink(pathname); }

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
