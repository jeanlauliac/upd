#include "run_command_line.h"
#include "io/io.h"
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
  std::stringbuf result;
  ssize_t count;
  do {
    char buffer[1 << 12];
    try {
      count = io::read(fd, buffer, sizeof(buffer));
    } catch (const std::system_error &error) {
      // On Linux, EIO is returned when the last
      // slave of a pseudo-terminal is closed.
      if (allow_eio && error.code() == std::errc::io_error) return result.str();
      throw;
    }
    result.sputn(buffer, count);
  } while (count > 0);
  return result.str();
}

/**
 * `posix_spawn()` to run a command line.
 */
command_line_result run_command_line(const command_line &target,
                                     int stderr_read_fd,
                                     const std::string &stderr_pts) {
  system::string_vector argv;
  argv.push_back(target.binary_path);
  for (auto const &arg : target.args) {
    argv.push_back(arg);
  }

  int stdout[2];
  io::pipe(stdout);

  int stderr_fd = io::open(stderr_pts.c_str(), O_WRONLY | O_NOCTTY, 0);
  if (!io::isatty(stderr_fd)) throw std::runtime_error("stderr is not a tty");

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

  system::string_vector env;
  env.push_back("TERM=xterm-color");
  for (const auto &var : target.environment) {
    env.push_back(var.first + "=" + var.second);
  }

  pid_t child_pid = system::spawn(target.binary_path, actions, argv, env);
  actions.destroy();

  io::close(stdout[1]);
  io::close(stderr_fd);

  int status;
  io::waitpid(child_pid, &status, 0);

  command_line_result result = {
      read_stdout.get(),
      read_stderr.get(),
      status,
  };

  io::close(stdout[0]);

  return result;
}

} // namespace upd
