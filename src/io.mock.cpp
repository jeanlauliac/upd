#include "io.h"

namespace upd {
namespace io {

const char *UPDFILE_SUFFIX = "/updfile.json";

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

int open(const std::string &, int, mode_t) { return 3; }

size_t write(int, const void *, size_t) { return 0; }

void close(int) {}

} // namespace io
} // namespace upd
