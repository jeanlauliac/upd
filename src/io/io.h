#pragma once

#include "../../gen/src/io/io.h"
#include <dirent.h>
#include <iostream>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

namespace upd {
namespace io {

/**
 * Get the current working directory.
 */
std::string getcwd();

bool is_regular_file(const std::string &path);

struct ifstream_failed_error {
  ifstream_failed_error(const std::string &file_path_)
      : file_path(file_path_) {}
  const std::string file_path;
};

template <typename ostream_t> struct stream_string_joiner {
  stream_string_joiner(ostream_t &os, const std::string &separator)
      : os_(os), first_(true), separator_(separator) {}
  template <typename elem_t>
  stream_string_joiner<ostream_t> &push(const elem_t &elem) {
    if (!first_) os_ << separator_;
    os_ << elem;
    first_ = false;
    return *this;
  }

private:
  ostream_t &os_;
  bool first_;
  std::string separator_;
};

DIR *opendir(const char *name) noexcept;
struct dirent *readdir(DIR *dirp) noexcept;
int closedir(DIR *dirp) noexcept;

/**
 * Creates a FIFO (named pipe) with the specified name.
 */
void mkfifo(const std::string &file_path, mode_t mode);

char *mkdtemp(char *tpl) noexcept;

int mkdir(const char *path, mode_t mode) noexcept;

/**
 * Open a file. Returned number is a valid file descriptor.
 */
int open(const std::string &file_path, int flags, mode_t mode);

int mkfifo(const char *path, mode_t mode) noexcept;

size_t write(int fd, const void *buf, size_t count);

ssize_t read(int fd, void *buf, size_t count);

void close(int fd);

int lstat(const char *path, struct ::stat *buf) noexcept;

int posix_openpt(int oflag);
void grantpt(int fd);
void unlockpt(int fd);
std::string ptsname(int fd);

void pipe(int pipefd[2]);

int isatty(int fd);

void posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions,
                                       int fd);
void posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions,
                                      int fd, int new_fd);
void posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *actions,
                                      int fd, const char *file_path, int oflag,
                                      mode_t mode);
void posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *actions);
void posix_spawn_file_actions_init(posix_spawn_file_actions_t *actions);

void posix_spawn(pid_t *pid, const char *path,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp, char *const argv[],
                 char *const envp[]);

pid_t waitpid(pid_t pid, int *status, int options);

namespace mock {

typedef std::vector<spawn_record> spawn_records_t;
extern spawn_records_t spawn_records;

void reset();
void register_binary(const std::string &binary_path, const std::string &stdout,
                     const std::string &stderr);

} // namespace mock

} // namespace io
} // namespace upd
