#pragma once
#include "file_descriptor.h"

namespace upd {
namespace io {

struct pseudoterminal {
  pseudoterminal();

  int fd() const { return fd_; }
  const std::string &ptsname() const { return ptsname_; }

private:
  io::file_descriptor fd_;
  std::string ptsname_;
};

} // namespace io
} // namespace upd