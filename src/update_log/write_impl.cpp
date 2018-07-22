#include "write_impl.h"

namespace upd {
namespace update_log {

void write_var_size_t(std::vector<char> &buffer, size_t value) {
  do {
    unsigned char next = value & 127;
    value >>= 7;
    if (value > 0) next |= 128;
    write_scalar(buffer, next);
  } while (value > 0);
}

void write_string(std::vector<char> &buffer, const std::string &value) {
  write_var_size_t(buffer, value.size());
  auto size = buffer.size();
  buffer.resize(size + value.size());
  std::memcpy(&buffer[size], &value[0], value.size());
}

} // namespace update_log
} // namespace upd
