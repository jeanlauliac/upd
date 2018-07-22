#pragma once

#include "read.h"

namespace upd {
namespace update_log {

template <typename Read, typename Scalar>
bool try_read_scalar(Read &&read, Scalar &value) {
  char *ptr = reinterpret_cast<char *>(&value);
  auto bytes_read = read(ptr, sizeof(Scalar));
  if (bytes_read == 0) return false;
  if (bytes_read < sizeof(Scalar)) throw unexpected_end_of_file_error();
  return true;
}

template <typename Read, typename Scalar>
void read_scalar(Read &&read, Scalar &value) {
  char *ptr = reinterpret_cast<char *>(&value);
  if (read(ptr, sizeof(Scalar)) < sizeof(Scalar))
    throw unexpected_end_of_file_error();
}

template <typename Read> void read_var_size_t(Read &&read, size_t &value) {
  value = 0;
  unsigned char next;
  size_t shift = 0;
  size_t count = 5;
  do {
    read_scalar(read, next);
    value |= (next & 127) << shift;
    shift += 7;
    --count;
  } while ((next & 128) > 0 && count > 0);
  if (count == 0) throw std::runtime_error("invalid var size_t");
}

template <typename Read> void read_string(Read &&read, std::string &value) {
  size_t size;
  read_var_size_t(read, size);
  value.resize(size);
  if (read(&value[0], size) < size) throw unexpected_end_of_file_error();
}

} // namespace update_log
} // namespace upd
