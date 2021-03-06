#include "utils.h"
#include "file_descriptor.h"
#include <array>
#include <cstring>
#include <fcntl.h>
#include <sstream>

namespace upd {
namespace io {

void throw_errno() { throw std::system_error(errno, std::generic_category()); }

void mkdir_s(const std::string &dir_path, mode_t mode) {
  if (io::mkdir(dir_path.c_str(), mode) != 0) throw_errno();
}

std::string mkdtemp_s(const std::string &template_path) {
  std::vector<char> tpl(template_path.size() + 1);
  std::strcpy(tpl.data(), template_path.c_str());
  if (io::mkdtemp(tpl.data()) == nullptr) throw_errno();
  return tpl.data();
}

constexpr size_t BLOCK_SIZE = 1 << 12;

std::string read_entire_file(const std::string &file_path) {
  file_descriptor fd = io::open(file_path, O_RDONLY, 0);
  std::stringbuf sb;
  std::array<char, BLOCK_SIZE> buffer;
  size_t bytes_read;
  do {
    bytes_read = io::read(fd, buffer.data(), buffer.size());
    sb.sputn(buffer.data(), bytes_read);
  } while (bytes_read == BLOCK_SIZE);
  return sb.str();
}

void write_entire_file(const std::string &file_path,
                       const std::string &content) {
  file_descriptor fd = io::open(file_path, O_WRONLY | O_CREAT, S_IRWXU);
  for (size_t i = 0; i < file_path.size(); i += BLOCK_SIZE) {
    auto slice = content.substr(i, BLOCK_SIZE);
    io::write(fd, slice.c_str(), slice.size());
  }
}

dir::dir(const std::string &path) : ptr_(io::opendir(path.c_str())) {
  if (ptr_ == nullptr) throw std::runtime_error("opendir() failed");
}

dir::dir() : ptr_(nullptr) {}

dir::~dir() {
  if (ptr_ != nullptr) io::closedir(ptr_);
}

void dir::open(const std::string &path) {
  if (ptr_ != nullptr) io::closedir(ptr_);
  ptr_ = io::opendir(path.c_str());
  if (ptr_ == nullptr) throw std::runtime_error("opendir() failed");
}

void dir::close() {
  io::closedir(ptr_);
  ptr_ = nullptr;
}

dir_files_reader::dir_files_reader(const std::string &path) : target_(path) {}
dir_files_reader::dir_files_reader() {}

struct dirent *dir_files_reader::next() {
  if (target_.ptr() == nullptr) throw std::runtime_error("no dir is open");
  errno = 0;
  return io::readdir(target_.ptr());
}

void dir_files_reader::open(const std::string &path) { target_.open(path); }

void dir_files_reader::close() { target_.close(); }

} // namespace io
} // namespace upd
