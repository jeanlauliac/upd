#include "command_line_runner.h"
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace upd {

/**
 * `fork()`/`exec()` to run a command line. We cannot use `posix_spawn()`
 * (faster on macOS) because we need to be able to `chdir()` in the child in a
 * thread-safe manner. If really it proves slow, `clone()` may be investigated.
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
  pid_t child_pid = fork();
  if (child_pid == 0) {
    close(depfile_fds[0]);
    if (chdir(root_path.c_str()) != 0) {
      std::cerr << "upd: *** chdir() failed in child process" << std::endl;
      _exit(127);
    }
    execvp(target.binary_path.c_str(), argv.data());
    std::cerr << "upd: *** execvp() failed in child process" << std::endl;
    _exit(127);
  }
  if (child_pid < 0) {
    throw std::runtime_error("command line failed");
  }
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
