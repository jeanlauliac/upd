#include "read.h"
#include "read_fd_forward.h"
#include "read_impl.h"
#include <algorithm>

namespace upd {
namespace update_log {

template <typename Read>
void read_ent_path(const string_vector &ent_paths, Read &read,
                   std::string &value) {
  size_t ent_id;
  read_var_size_t(read, ent_id);
  value = ent_paths.at(ent_id);
}

template <typename Read>
bool read_update_record(const string_vector &ent_paths, Read &&read,
                        std::string &file_name, file_record &record) {
  read_scalar(read, record.imprint);
  read_scalar(read, record.hash);
  read_ent_path(ent_paths, read, file_name);
  uint16_t dep_count;
  read_scalar(read, dep_count);
  record.dependency_local_paths.resize(dep_count);
  for (size_t i = 0; i < dep_count; ++i)
    read_ent_path(ent_paths, read, record.dependency_local_paths[i]);
  return true;
}

template <typename Read>
std::pair<size_t, std::string> read_entity_name_record(Read &&read) {
  std::pair<size_t, std::string> record;
  read_var_size_t(read, record.first);
  read_string(read, record.second);
  return record;
}

/**
 * `Read` is a function such as `size_t()(char* buffer, size_t count)` that
 * reads `count` bytes from some data source into the `buffer`, and return the
 * number of bytes actually read (ex. if we reached the end of a file).
 */
template <typename Read> cache_file_data read(Read &&read) {
  cache_file_data rs;
  record_type type;
  char version;
  read_scalar(read, version);
  if (version != VERSION) throw version_mismatch_error();
  while (try_read_scalar(read, type)) {
    if (type == record_type::file_update) {
      file_record record;
      std::string file_path;
      read_update_record(rs.ent_paths, read, file_path, record);
      rs.records[file_path] = record;
      continue;
    }
    if (type == record_type::root_entity_name) {
      std::string name;
      read_string(read, name);
      rs.ent_paths.push_back(name);
      continue;
    }
    if (type == record_type::entity_name) {
      auto record = read_entity_name_record(read);
      auto parent_path = rs.ent_paths.at(record.first) + "/";
      rs.ent_paths.push_back(parent_path + record.second);
      continue;
    }
    throw std::runtime_error("wrong record type: " +
                             std::to_string(static_cast<unsigned char>(type)));
  }
  return rs;
}

cache_file_data read_fd(int fd) { return read(read_fd_forward<4096>(fd)); }

} // namespace update_log
} // namespace upd
