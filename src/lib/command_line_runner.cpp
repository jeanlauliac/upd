#include "command_line_runner.h"
#include <iostream>
#include <spawn.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace upd {

/**
 * `posix_spawn()` to run a command line.
 */
void command_line_runner::run(
  const std::string& root_path,
  command_line target,
  int depfile_fds[2]
) {
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(target.binary_path.c_str()));
  for (auto const& arg: target.args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    throw std::runtime_error("action init failed");
  }
  if (posix_spawn_file_actions_addclose(&actions, depfile_fds[0]) != 0) {
    throw std::runtime_error("action addclose failed");
  };

  pid_t child_pid;
  int res = posix_spawnp(
    &child_pid,
    target.binary_path.c_str(),
    &actions,
    nullptr,
    argv.data(),
    environ
  );
  if (res != 0) {
    throw std::runtime_error("command line failed");
  }
  if (posix_spawn_file_actions_destroy(&actions) != 0) {
    throw std::runtime_error("action destroy failed");
  };
  close(depfile_fds[1]);

  int status;
  if (waitpid(child_pid, &status, 0) != child_pid) {
    throw std::runtime_error("waitpid failed");
  }
  if (!WIFEXITED(status)) {
    throw std::runtime_error("process did not terminate normally");
  }
  if (WEXITSTATUS(status) != 0) {
    throw std::runtime_error("process terminated with errors");
  }
}

}
