#pragma once

#include <spawn.h>
#include <string>

namespace upd {
namespace system {

struct errno_error {
  errno_error(int code) : code(code) {}
  int code;
};

/**
 * Wrapper around the `posix_spawn_file_actions_*` functions that handles
 * destroying the data structure automatically.
 */
struct spawn_file_actions {
  spawn_file_actions();
  ~spawn_file_actions();
  spawn_file_actions(spawn_file_actions &) = delete;
  spawn_file_actions(spawn_file_actions &&);

  void add_close(int fd);
  void add_dup2(int fd, int newfd);
  void destroy();
  const posix_spawn_file_actions_t &posix() const { return pdfa_; }

private:
  posix_spawn_file_actions_t pdfa_;
  bool init_;
};

/**
 * Wrapper around `posix_spawn` that throws in case of error.
 */
int spawn(const std::string binary_path, const spawn_file_actions &actions,
          char *const *argv, char *const *env);

} // namespace system
} // namespace upd
