#pragma once

namespace upd {
namespace system {

struct errno_error {
  errno_error(int code_) : code(code_) {}
  const int code;
};

} // namespace system
} // namespace upd
