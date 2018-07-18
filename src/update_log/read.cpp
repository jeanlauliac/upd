#include "read.h"
#include <algorithm>

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

template <typename Read>
void read_var_size_t(Read &&read, size_t& value) {
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

template <typename Read>
void read_ent_path(const string_vector& ent_paths, Read &read,
                   std::string &value) {
  size_t ent_id;
  read_var_size_t(read, ent_id);
  value = ent_paths.at(ent_id);
}

template <typename Read>
bool read_update_record(const string_vector& ent_paths, Read &&read,
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
    throw std::runtime_error("wrong record type: " + std::to_string(static_cast<unsigned char>(type)));
  }
  return rs;
}

struct read_fd_forward {
  read_fd_forward(size_t fd) : fd_(fd), next_(buf_.data()), end_(buf_.data()) {}
  size_t operator()(char *buf, size_t count) {
    auto read_count = fill_(buf, count);
    while (read_count < count && read_()) {
      read_count += fill_(buf, count - read_count);
    }
    return read_count;
  }

private:
  size_t fill_(char *buf, size_t count) {
    count = std::min(count, static_cast<size_t>(end_ - next_));
    std::memcpy(buf, next_, count);
    next_ += count;
    return count;
  }

  bool read_() {
    auto read_count = io::read(fd_, buf_.data(), buf_.size());
    next_ = buf_.data();
    end_ = buf_.data() + read_count;
    return read_count > 0;
  }

  size_t fd_;
  std::array<char, 1 << 12> buf_;
  char *next_;
  char *end_;
};

cache_file_data read_fd(int fd) { return read(read_fd_forward(fd)); }

} // namespace update_log
} // namespace upd
