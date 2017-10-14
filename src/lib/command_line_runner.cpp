#include "command_line_runner.h"
#include <future>
#include <iostream>
#include <spawn.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace upd {

/**
 * Read a file descriptor until the end is reached and return the content
 * as a single string.
 */
static std::string read_fd_to_string(int fd) {
  std::ostringstream result;
  result.exceptions(std::ostringstream::badbit | std::ostringstream::failbit);
  ssize_t count;
  do {
    char buffer[1 << 12];
    count = read(fd, buffer, sizeof(buffer));
    if (count < 0) {
      throw std::runtime_error("read() failed");
    }
    result.write(buffer, count);
  } while (count > 0);
  return result.str();
}

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
  int stdout[2];
  if (pipe(stdout) != 0) throw std::runtime_error("pipe() failed");
  int stderr[2];
  if (pipe(stderr) != 0) throw std::runtime_error("pipe() failed");

  if (posix_spawn_file_actions_addclose(&actions, depfile_fds[0]) != 0) {
    throw std::runtime_error("action addclose failed");
  }
  if (posix_spawn_file_actions_addclose(&actions, stdout[0]) != 0) {
    throw std::runtime_error("action addclose failed");
  }
  if (posix_spawn_file_actions_addclose(&actions, stderr[0]) != 0) {
    throw std::runtime_error("action addclose failed");
  }
  if (posix_spawn_file_actions_adddup2(&actions, stdout[1], STDOUT_FILENO) != 0) {
    throw std::runtime_error("action adddup2 failed");
  }
  if (posix_spawn_file_actions_adddup2(&actions, stderr[1], STDERR_FILENO) != 0) {
    throw std::runtime_error("action adddup2 failed");
  }
  if (posix_spawn_file_actions_addclose(&actions, stdout[1]) != 0) {
    throw std::runtime_error("action addclose failed");
  }
  if (posix_spawn_file_actions_addclose(&actions, stderr[1]) != 0) {
    throw std::runtime_error("action addclose failed");
  }

  auto read_stdout = std::async(std::launch::async, &read_fd_to_string, stdout[0]);
  auto read_stderr = std::async(std::launch::async, &read_fd_to_string, stderr[0]);

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
  if (close(depfile_fds[1]) != 0) throw std::runtime_error("close() failed");
  if (close(stdout[1])) throw std::runtime_error("close() failed");
  if (close(stderr[1])) throw std::runtime_error("close() failed");

  int status;
  if (waitpid(child_pid, &status, 0) != child_pid) {
    throw std::runtime_error("waitpid failed");
  }

  std::cerr << read_stdout.get();
  std::cerr << read_stderr.get();

  if (close(stdout[0])) throw std::runtime_error("close() failed");
  if (close(stderr[0])) throw std::runtime_error("close() failed");

  if (!WIFEXITED(status)) {
    throw std::runtime_error("process did not terminate normally");
  }
  if (WEXITSTATUS(status) != 0) {
    throw std::runtime_error("process terminated with errors");
  }
}

}
