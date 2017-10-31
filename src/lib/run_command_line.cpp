#include "run_command_line.h"
#include "system/spawn.h"
#include <fcntl.h>
#include <future>
#include <iostream>
#include <spawn.h>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
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
static std::string read_fd_to_string(int fd, bool allow_eio) {
  std::ostringstream result;
  result.exceptions(std::ostringstream::badbit | std::ostringstream::failbit);
  ssize_t count;
  do {
    char buffer[1 << 12];
    count = read(fd, buffer, sizeof(buffer));
    if (count < 0) {
      if (allow_eio && errno == EIO) {
        // On Linux, EIO is returned when the last
        // slave of a pseudo-terminal is closed.
        return result.str();
      }
      throw std::runtime_error("read() failed: " + std::to_string(errno));
    }
    result.write(buffer, count);
  } while (count > 0);
  return result.str();
}

/**
 * `posix_spawn()` to run a command line.
 */
command_line_result run_command_line(const command_line &target,
                                     int stderr_read_fd,
                                     const std::string &stderr_pts) {
  std::vector<char *> argv;
  argv.push_back(const_cast<char *>(target.binary_path.c_str()));
  for (auto const &arg : target.args) {
    argv.push_back(const_cast<char *>(arg.c_str()));
  }
  argv.push_back(nullptr);

  int stdout[2];
  if (pipe(stdout) != 0) throw std::runtime_error("pipe() failed");

  int stderr_fd = open(stderr_pts.c_str(), O_WRONLY | O_NOCTTY);
  if (stderr_fd < 0) throw std::runtime_error("open() for stderr failed");
  if (!isatty(stderr_fd)) throw std::runtime_error("stderr is not a tty");

  system::spawn_file_actions actions;

  actions.add_close(stdout[0]);
  actions.add_dup2(stdout[1], STDOUT_FILENO);
  actions.add_close(stdout[1]);

  actions.add_close(stderr_read_fd);
  actions.add_dup2(stderr_fd, STDERR_FILENO);
  actions.add_close(stderr_fd);

  auto read_stdout =
      std::async(std::launch::async, &read_fd_to_string, stdout[0], false);
  auto read_stderr =
      std::async(std::launch::async, &read_fd_to_string, stderr_read_fd, true);

  pid_t child_pid =
      system::spawn(target.binary_path, actions, argv.data(), environ);
  actions.destroy();

  if (close(stdout[1]) != 0) throw std::runtime_error("close() failed");
  if (close(stderr_fd) != 0) throw std::runtime_error("close() failed");

  int status;
  if (waitpid(child_pid, &status, 0) != child_pid) {
    throw std::runtime_error("waitpid failed");
  }

  command_line_result result = {
      read_stdout.get(),
      read_stderr.get(),
      status,
  };

  if (close(stdout[0])) throw std::runtime_error("close() failed");

  return result;
}

} // namespace upd
