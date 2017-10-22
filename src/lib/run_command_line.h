#pragma once

#include "command_line_template.h"
#include <string>

namespace upd {

struct command_line_result {
  std::string stdout;
  std::string stderr;
  int status;
};

command_line_result run_command_line(
  const std::string& root_path,
  const command_line& target,
  int depfile_fds[2]
);

}
