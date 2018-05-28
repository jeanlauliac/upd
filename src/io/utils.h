#pragma once

#include "io.h"
#include <string>

namespace upd {
namespace io {

/**
 * Throw a `system_error` with the current `errno` error code.
 */
void throw_errno();

void mkdir_s(const std::string &dir_path, mode_t mode);

std::string read_entire_file(const std::string &file_path);
void write_entire_file(const std::string &file_path,
                       const std::string &content);

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

} // namespace io
} // namespace upd
