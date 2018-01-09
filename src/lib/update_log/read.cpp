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
bool read_update_record(Reader &reader, std::string &file_name,
                        file_record &record) {
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

template <typename Reader>
std::pair<uint16_t, std::string> read_entity_name_record(Reader &reader) {
  std::pair<uint16_t, std::string> record;
  read_scalar(reader, record.first);
  read_string(reader, record.second);
  return record;
}

template <typename Reader> cache_file_data read(Reader &reader) {
  records_by_file records;
  string_vector ent_paths;
  record_type type;
  while (try_read_scalar(reader, type)) {
    if (type == record_type::file_update) {
      file_record record;
      std::string file_path;
      read_update_record(reader, file_path, record);
      records[file_path] = record;
      continue;
    }
    if (type == record_type::entity_name) {
      auto record = read_entity_name_record(reader);
      auto parent_path = record.first != static_cast<uint16_t>(-1)
                             ? ent_paths.at(record.first)
                             : "";
      ent_paths.push_back(parent_path + '/' + record.second);
      continue;
    }
    throw std::runtime_error("wrong record type");
  }
  return {records, ent_paths};
}

template cache_file_data read(string_char_reader &);
template cache_file_data read(fd_char_reader &);

} // namespace update_log
} // namespace upd
