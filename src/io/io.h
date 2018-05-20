#pragma once

#include "../../gen/src/io/io.h"
#include <dirent.h>
#include <iostream>
#include <spawn.h>
#include <sys/types.h>
#include <vector>

namespace upd {
namespace io {

/**
 * Get the current working directory.
 */
std::string getcwd();

bool is_regular_file(const std::string &path);

/**
 * Keep track and automatically delete a directory handle.
 */
struct dir {
  dir(const std::string &path);
  dir();
  dir(dir &) = delete;
  ~dir();
  void open(const std::string &path);
  void close();
  bool is_open() { return ptr_ != nullptr; }
  DIR *ptr() const { return ptr_; }

private:
  DIR *ptr_;
};

/**
 * Provide us with all the files+subfolders of a folder.
 */
struct dir_files_reader {
  dir_files_reader(const std::string &path);
  dir_files_reader();
  /**
   * The returned dirent pointer should never be released manually. It is
   * automatically released by the system on the next call, or object
   * destruction.
   */
  struct dirent *next();
  void open(const std::string &path);
  void close();
  bool is_open() { return target_.is_open(); }

private:
  dir target_;
};

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

/**
 * Create a temporary folder. `template_path` must be a full path, for example
 * `/tmp/foo.XXXXXX`, where "X"s are replaced by random unique characters.
 */
std::string mkdtemp(const std::string &template_path);

/**
 * Creates a FIFO (named pipe) with the specified name.
 */
void mkfifo(const std::string &file_path, mode_t mode);

void mkdir(const std::string &dir_path, mode_t mode);

/**
 * Open a file. Returned number is a valid file descriptor.
 */
int open(const std::string &file_path, int flags, mode_t mode);

size_t write(int fd, const void *buf, size_t count);

ssize_t read(int fd, void *buf, size_t count);

void close(int fd);

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
