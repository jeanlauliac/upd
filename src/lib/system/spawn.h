#pragma once

#include <spawn.h>
#include <string>
#include <vector>

namespace upd {
namespace system {

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
 * Custom string vector for passing as argument to spawn(). This has the
 * benefit of having a `data()` function that directly returns a `char**`.
 */
struct string_vector {
  string_vector();
  ~string_vector();
  string_vector(string_vector &) = delete;
  string_vector(string_vector &&) = delete;

  void push_back(const std::string &value);
  char **data();

private:
  std::vector<char *> v_;
};

/**
 * Wrapper around `posix_spawn` that throws in case of error.
 */
int spawn(const std::string binary_path, const spawn_file_actions &actions,
          string_vector &argv, string_vector &env);

} // namespace system
} // namespace upd
