#include "read.h"

namespace upd {
namespace update_log {

template <typename Reader> void read_char(Reader &reader, char &c) {
  if (!reader.next(c)) throw unexpected_end_of_file_error();
}

template <typename Reader, typename Scalar>
bool try_read_scalar(Reader &reader, Scalar &value) {
  char *ptr = reinterpret_cast<char *>(&value);
  if (!reader.next(*ptr)) return false;
  ++ptr;
  for (size_t i = 1; i < sizeof(Scalar); ++i, ++ptr) read_char(reader, *ptr);
  return true;
}

template <typename Reader, typename Scalar>
void read_scalar(Reader &reader, Scalar &value) {
  char *ptr = reinterpret_cast<char *>(&value);
  for (size_t i = 0; i < sizeof(Scalar); ++i, ++ptr) read_char(reader, *ptr);
}

template <typename Reader>
void read_string(Reader &reader, std::string &value) {
  uint16_t size;
  read_scalar(reader, size);
  value.resize(size);
  for (size_t i = 0; i < size; ++i) read_char(reader, value[i]);
}

template <typename Reader>
bool read_record(Reader &reader, std::string &file_name, file_record &record) {
  record_type type;
  if (!try_read_scalar(reader, type)) return false;
  if (type != record_type::file_update) {
    throw std::runtime_error("wrong record type");
  }
  read_scalar(reader, record.imprint);
  read_scalar(reader, record.hash);
  read_string(reader, file_name);
  uint16_t dep_count;
  read_scalar(reader, dep_count);
  record.dependency_local_paths.resize(dep_count);
  for (size_t i = 0; i < dep_count; ++i)
    read_string(reader, record.dependency_local_paths[i]);
  return true;
}

template <typename Reader> records_by_file read(Reader &reader) {
  records_by_file result;
  file_record record;
  std::string file_path;
  while (read_record(reader, file_path, record)) {
    result[file_path] = record;
  }
  return result;
}

template records_by_file read(string_char_reader &);
template records_by_file read(fd_char_reader &);

} // namespace update_log
} // namespace upd
