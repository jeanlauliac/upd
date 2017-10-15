#pragma once

#include "command_line_template.h"
#include <string>

namespace upd {

struct command_line_result {
  std::string stdout;
  std::string stderr;
  int status;
};

struct command_line_runner {
  static command_line_result run(
    const std::string& root_path,
    command_line target,
    int depfile_fds[2]
  );
};

}
