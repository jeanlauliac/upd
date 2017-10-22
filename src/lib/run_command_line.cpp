#include "run_command_line.h"
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

struct errno_error {
  errno_error(int code): code(code) {}
  int code;
};

struct spawn_file_actions {
  spawn_file_actions();
  ~spawn_file_actions();
  spawn_file_actions(spawn_file_actions&) = delete;
  spawn_file_actions(spawn_file_actions&&) = delete;

  void add_close(int fd);
  void add_dup2(int fd, int newfd);
  void destroy();
  const posix_spawn_file_actions_t& posix() const { return pdfa_; }

private:
  posix_spawn_file_actions_t pdfa_;
  bool init_;
};

spawn_file_actions::spawn_file_actions(): init_(true) {
  if (posix_spawn_file_actions_init(&pdfa_) == 0) return;
  throw errno_error(errno);
}

spawn_file_actions::~spawn_file_actions() {
  destroy();
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

static int spawn(
  const std::string binary_path,
  const spawn_file_actions& actions,
  char* const* argv,
  char* const* env
) {
  pid_t pid;
  auto bin = binary_path.c_str();
  auto pa = &actions.posix();
  int res = posix_spawnp(&pid, bin, pa, nullptr, argv, env);
  if (res == 0) return pid;
  throw errno_error(errno);
}

/**
 * `posix_spawn()` to run a command line.
 */
command_line_result run_command_line(
  const std::string& root_path,
  const command_line& target,
  int depfile_fds[2]
) {
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(target.binary_path.c_str()));
  for (auto const& arg: target.args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  int stdout[2];
  if (pipe(stdout) != 0) throw std::runtime_error("pipe() failed");
  int stderr[2];
  if (pipe(stderr) != 0) throw std::runtime_error("pipe() failed");

  spawn_file_actions actions;
  actions.add_close(depfile_fds[0]);
  actions.add_close(stdout[0]);
  actions.add_close(stderr[0]);
  actions.add_dup2(stdout[1], STDOUT_FILENO);
  actions.add_dup2(stderr[1], STDERR_FILENO);
  actions.add_close(stdout[1]);
  actions.add_close(stderr[1]);

  auto read_stdout =
    std::async(std::launch::async, &read_fd_to_string, stdout[0]);
  auto read_stderr =
    std::async(std::launch::async, &read_fd_to_string, stderr[0]);

  pid_t child_pid = spawn(target.binary_path, actions, argv.data(), environ);
  actions.destroy();

  if (close(depfile_fds[1]) != 0) throw std::runtime_error("close() failed");
  if (close(stdout[1])) throw std::runtime_error("close() failed");
  if (close(stderr[1])) throw std::runtime_error("close() failed");

  int status;
  if (waitpid(child_pid, &status, 0) != child_pid) {
    throw std::runtime_error("waitpid failed");
  }

  command_line_result result = {
    .stdout = read_stdout.get(),
    .stderr = read_stderr.get(),
    .status = status,
  };

  if (close(stdout[0])) throw std::runtime_error("close() failed");
  if (close(stderr[0])) throw std::runtime_error("close() failed");

  return result;
}

}
