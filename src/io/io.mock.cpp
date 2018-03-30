#include "io.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace upd {
namespace io {

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

struct fd_data {
  std::string file_path;
  size_t position;
};

std::unordered_map<std::string, std::vector<char>> files;
std::unordered_map<size_t, fd_data> fds;

void reset_mock() {
  files.clear();
  fds.clear();
}

size_t alloc_fd() {
  int fd = 3;
  while (fds.find(fd) != fds.end() && fd < 1024) ++fd;
  if (fd == 1024) throw std::system_error(EMFILE, std::generic_category());
  return fd;
}

int open(const std::string &file_path, int flags, mode_t) {
  if (files.find(file_path) == files.end()) {
    if ((flags & O_CREAT) == 0) {
      throw std::system_error(ENOENT, std::generic_category());
    }
    files[file_path] = {};
  }
  auto fd = alloc_fd();
  fds[fd] = {file_path, 0};
  return fd;
}

size_t write(int fd, const void *buf, size_t size) {
  auto &desc = fds[fd];
  auto &file_buf = files[desc.file_path];
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
  auto &file_buf = files[desc.file_path];
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

std::string ptsname(int fd) { return "/pseudoterminal/" + std::to_string(fd); }

} // namespace io
} // namespace upd
