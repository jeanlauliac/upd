#pragma once

#include "command_line_template.h"
#include <string>

namespace upd {

struct command_line_result {
  std::string stdout;
  std::string stderr;
  int status;
};

command_line_result run_command_line(const command_line &target,
                                     int stderr_read_fd,
                                     const std::string &stderr_pts);

} // namespace upd
