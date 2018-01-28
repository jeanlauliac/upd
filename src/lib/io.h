#pragma once

#include <dirent.h>
#include <iostream>
#include <sys/types.h>

namespace upd {
namespace io {

extern const char *ROOTFILE_SUFFIX;
extern const char *UPDFILE_SUFFIX;

/**
 * Get the current working directory.
 */
std::string getcwd();

/**
 * Thrown when no directory could be found containing a `.updroot` file,
 * searching all the current working directory and its parents.
 */
struct cannot_find_root_error {};

/**
 * Figure out the root directory of the project. It is identified by containing
 * a file named `.updroot`. This path is used as the base for what we call
 * "local paths". All these local paths are canonicalized in terms of the root
 * path.
 */
std::string find_root_path(std::string origin_path);

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

/**
 * Open a file. Returned number is a valid file descriptor.
 */
int open(const std::string &file_path, int flags, mode_t mode);

size_t write(int fd, const void *buf, size_t count);

void close(int fd);

} // namespace io
} // namespace upd
