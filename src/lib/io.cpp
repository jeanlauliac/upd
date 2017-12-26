#include "io.h"
#include "system/errno_error.h"
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

const char *ROOTFILE_SUFFIX = "/.updroot";
const char *UPDFILE_SUFFIX = "/updfile.json";

std::string getcwd() {
  char temp[MAXPATHLEN];
  if (::getcwd(temp, MAXPATHLEN) == nullptr) {
    throw system::errno_error(errno);
  }
  return temp;
}

std::string dirname_string(const std::string &path) {
  if (path.size() >= MAXPATHLEN) {
    throw std::runtime_error("string too long");
  }
  char temp[MAXPATHLEN];
  std::strcpy(temp, path.c_str());
  return dirname(temp);
}

static bool is_regular_file(const std::string &path) {
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

std::string find_root_path(std::string path) {
  bool found = is_regular_file(path + ROOTFILE_SUFFIX);
  while (!found && path != "/") {
    path = dirname_string(path);
    found = is_regular_file(path + ROOTFILE_SUFFIX);
  }
  if (!found) throw cannot_find_root_error();
  return path;
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
  if (::mkdtemp(tpl.data()) != NULL) return tpl.data();
  throw system::errno_error(errno);
}

void mkfifo(const std::string &file_path, mode_t mode) {
  if (::mkfifo(file_path.c_str(), mode) == 0) return;
  throw system::errno_error(errno);
}

int open(const std::string &file_path, int flags) {
  int fd = ::open(file_path.c_str(), flags);
  if (fd < 0) {
    throw system::errno_error(errno);
  }
  return fd;
}

} // namespace io
} // namespace upd
