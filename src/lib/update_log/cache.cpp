#include "cache.h"
#include "../fd_char_reader.h"
#include "../io.h"
#include "../system/errno_error.h"
#include "read.h"
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

#include "../inspect.h"

namespace upd {
namespace update_log {

template <typename Scalar>
void write_scalar(std::vector<char> &buffer, const Scalar &value) {
  auto size = buffer.size();
  buffer.resize(size + sizeof(value));
  std::memcpy(&buffer[size], &value, sizeof(value));
}

static void write_string(std::vector<char> &buffer, const std::string &value) {
  write_scalar(buffer, static_cast<uint16_t>(value.size()));
  auto size = buffer.size();
  buffer.resize(size + value.size());
  std::memcpy(&buffer[size], &value[0], value.size());
}

static constexpr int WRITE_FLAGS =
    /* no reads are done when recording */
    O_WRONLY |
    /* before each write we ensure we're at the end of the file */
    O_APPEND |
    /* ensure each write is flushed before we continue so as to
       be crash-resilient */
    O_SYNC;

static constexpr int MODE = S_IRUSR | S_IWUSR;

recorder::recorder(const std::string &file_path)
    : fd_(io::open(file_path, O_CREAT | O_TRUNC | WRITE_FLAGS, MODE)) {}

static ent_ids_by_path build_ent_index(const string_vector &ent_names) {
  ent_ids_by_path index;
  for (size_t i = 0; i < ent_names.size(); ++i) {
    index[ent_names[i]] = i;
  }
  return index;
}

recorder::recorder(const std::string &file_path, const string_vector &ent_names)
    : fd_(io::open(file_path, WRITE_FLAGS, MODE)),
      ent_ids_by_path_(build_ent_index(ent_names)) {}

void recorder::record(const std::string &local_file_path,
                      const file_record &record) {
  std::vector<char> buf;
  write_scalar(buf, record_type::file_update);
  write_scalar(buf, record.imprint);
  write_scalar(buf, record.hash);
  get_path_id_(local_file_path);
  write_string(buf, local_file_path);
  write_scalar(buf,
               static_cast<uint16_t>(record.dependency_local_paths.size()));
  for (const auto &dep_path : record.dependency_local_paths) {
    get_path_id_(dep_path);
    write_string(buf, dep_path);
  }
  io::write(fd_, buf.data(), buf.size());
}

uint16_t recorder::get_path_id_(const std::string &file_path) {
  auto ix = file_path.find('/');
  uint16_t parent_ent_id = -1;
  size_t parent_ix = std::string::npos;
  do {
    auto path_part = file_path.substr(0, ix);
    auto idit = ent_ids_by_path_.find(path_part);
    if (idit != ent_ids_by_path_.end()) {
      parent_ent_id = idit->second;
    } else {
      auto ent_id = static_cast<uint16_t>(ent_ids_by_path_.size());
      ent_ids_by_path_.emplace(path_part, ent_id);
      auto ent_name = file_path.substr(parent_ix + 1, ix - parent_ix - 1);
      record_ent_name_(parent_ent_id, ent_name);
      parent_ent_id = ent_id;
    }
    parent_ix = ix;
    ix = file_path.find('/', ix + 1);
  } while (parent_ix != std::string::npos);
  return parent_ent_id;
}

void recorder::record_ent_name_(uint16_t parent_ent_id,
                                const std::string &name) {
  std::vector<char> buf;
  write_scalar(buf, record_type::entity_name);
  write_scalar(buf, parent_ent_id);
  write_string(buf, name);
  io::write(fd_, buf.data(), buf.size());
}

void recorder::close() { fd_.close(); }

cache::cache(const std::string &file_path, const cache_file_data &data)
    : recorder_(file_path, data.ent_names), cached_records_(data.records) {}

cache::cache(const std::string &file_path) : recorder_(file_path) {}

records_by_file::iterator cache::find(const std::string &local_file_path) {
  return cached_records_.find(local_file_path);
}

records_by_file::iterator cache::end() { return cached_records_.end(); }

void cache::record(const std::string &local_file_path,
                   const file_record &record) {
  recorder_.record(local_file_path, record);
  cached_records_[local_file_path] = record;
}

cache cache::from_log_file(const std::string &log_file_path) {
  file_descriptor fd;
  try {
    fd = io::open(log_file_path, O_RDONLY, 0);
  } catch (const system::errno_error &error) {
    if (error.code != ENOENT) throw;
    return cache(log_file_path);
  }
  fd_char_reader reader(fd);
  return cache(log_file_path, read(reader));
}

void rewrite_file(const std::string &file_path,
                  const std::string &temporary_file_path,
                  const records_by_file &records) {
  recorder fresh_recorder(temporary_file_path);
  for (auto record_entry : records) {
    fresh_recorder.record(record_entry.first, record_entry.second);
  }
  fresh_recorder.close();
  if (rename(temporary_file_path.c_str(), file_path.c_str()) != 0) {
    throw failed_to_rewrite_error();
  }
}

} // namespace update_log
} // namespace upd
