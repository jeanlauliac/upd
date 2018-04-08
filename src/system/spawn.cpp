#include "spawn.h"
#include "../io/io.h"
#include "errno_error.h"
#include <cstring>
#include <errno.h>
#include <iostream>

namespace upd {
namespace system {

spawn_file_actions::spawn_file_actions() : init_(true) {
  io::posix_spawn_file_actions_init(&pdfa_);
}

spawn_file_actions::~spawn_file_actions() { destroy(); }

spawn_file_actions::spawn_file_actions(spawn_file_actions &&other)
    : pdfa_(other.pdfa_), init_(other.init_) {
  other.init_ = false;
}

void spawn_file_actions::add_close(int fd) {
  io::posix_spawn_file_actions_addclose(&pdfa_, fd);
}

void spawn_file_actions::add_dup2(int fd, int newfd) {
  io::posix_spawn_file_actions_adddup2(&pdfa_, fd, newfd);
}

void spawn_file_actions::destroy() {
  if (!init_) return;
  init_ = false;
  io::posix_spawn_file_actions_destroy(&pdfa_);
}

string_vector::string_vector() : v_({nullptr}) {}

string_vector::~string_vector() {
  for (auto str : v_) {
    delete[] str;
  }
}

void string_vector::push_back(const std::string &value) {
  char *str = new char[value.size() + 1];
  strcpy(str, value.c_str());
  v_[v_.size() - 1] = str;
  v_.push_back(nullptr);
}

char **string_vector::data() { return v_.data(); }

int spawn(const std::string binary_path, const spawn_file_actions &actions,
          string_vector &argv, string_vector &env) {
  pid_t pid;
  auto bin = binary_path.c_str();
  auto pa = &actions.posix();
  io::posix_spawn(&pid, bin, pa, nullptr, argv.data(), env.data());
  return pid;
}

} // namespace system
} // namespace upd
