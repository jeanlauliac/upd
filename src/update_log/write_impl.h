#pragma once

#include <cstring>
#include <string>
#include <vector>

namespace upd {
namespace update_log {

template <typename Scalar>
void write_scalar(std::vector<char> &buffer, const Scalar &value) {
  auto size = buffer.size();
  buffer.resize(size + sizeof(value));
  std::memcpy(&buffer[size], &value, sizeof(value));
}

void write_var_size_t(std::vector<char> &buffer, size_t value);
void write_string(std::vector<char> &buffer, const std::string &value);

} // namespace update_log
} // namespace upd
