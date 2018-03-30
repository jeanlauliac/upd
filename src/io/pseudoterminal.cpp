#include "pseudoterminal.h"
#include <fcntl.h>

namespace upd {
namespace io {

pseudoterminal::pseudoterminal() {
  fd_ = io::posix_openpt(O_RDWR | O_NOCTTY);
  io::grantpt(fd_);
  io::unlockpt(fd_);
  ptsname_ = io::ptsname(fd_);
}

} // namespace io
} // namespace upd
