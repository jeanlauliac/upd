#include "spawn.h"
#include "errno_error.h"
#include <errno.h>
#include <iostream>

namespace upd {
namespace system {

spawn_file_actions::spawn_file_actions() : init_(true) {
  if (posix_spawn_file_actions_init(&pdfa_) == 0) return;
  throw errno_error(errno);
}

spawn_file_actions::~spawn_file_actions() { destroy(); }

spawn_file_actions::spawn_file_actions(spawn_file_actions &&other)
    : pdfa_(other.pdfa_), init_(other.init_) {
  other.init_ = false;
}

void spawn_file_actions::add_close(int fd) {
  if (posix_spawn_file_actions_addclose(&pdfa_, fd) == 0) return;
  throw errno_error(errno);
}

void spawn_file_actions::add_dup2(int fd, int newfd) {
  if (posix_spawn_file_actions_adddup2(&pdfa_, fd, newfd) == 0) return;
  throw errno_error(errno);
}

void spawn_file_actions::destroy() {
  if (!init_) return;
  init_ = false;
  if (posix_spawn_file_actions_destroy(&pdfa_) == 0) return;
  throw errno_error(errno);
}

int spawn(const std::string binary_path, const spawn_file_actions &actions,
          char *const *argv, char *const *env) {
  pid_t pid;
  auto bin = binary_path.c_str();
  auto pa = &actions.posix();
  int res = posix_spawn(&pid, bin, pa, nullptr, argv, env);
  if (res == 0) return pid;
  throw errno_error(errno);
}

} // namespace system
} // namespace upd
