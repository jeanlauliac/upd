#include "recorder.h"
#include "../io/io.h"
#include "read.h"
#include "write_impl.h"
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

namespace upd {
namespace update_log {

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
    : fd_(io::open(file_path, O_CREAT | O_TRUNC | WRITE_FLAGS, MODE)) {
  io::write(fd_, &VERSION, 1);
}

static ent_ids_by_path build_ent_index(const string_vector &ent_paths) {
  ent_ids_by_path index;
  for (size_t i = 0; i < ent_paths.size(); ++i) {
    index[ent_paths[i]] = i;
  }
  return index;
}

recorder::recorder(const std::string &file_path, const string_vector &ent_paths)
    : fd_(io::open(file_path, WRITE_FLAGS, MODE)),
      ent_ids_by_path_(build_ent_index(ent_paths)) {}

void recorder::record(const std::string &local_file_path,
                      const file_record &record) {
  std::vector<char> buf;
  write_scalar(buf, record_type::file_update);
  write_scalar(buf, record.imprint);
  write_scalar(buf, record.hash);
  write_var_size_t(buf, get_path_id_(local_file_path));
  write_var_size_t(buf, record.dependency_local_paths.size());
  for (const auto &dep_path : record.dependency_local_paths) {
    write_var_size_t(buf, get_path_id_(dep_path));
  }
  io::write(fd_, buf.data(), buf.size());
}

static const size_t no_id = ~0;

size_t recorder::get_path_id_(const std::string &file_path) {
  auto ix = file_path.find('/');
  size_t parent_ent_id = no_id;
  size_t parent_ix = std::string::npos;
  do {
    auto path_part = file_path.substr(0, ix);
    auto idit = ent_ids_by_path_.find(path_part);
    if (idit != ent_ids_by_path_.end()) {
      parent_ent_id = idit->second;
    } else {
      auto ent_id = ent_ids_by_path_.size();
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

void recorder::record_ent_name_(size_t parent_ent_id, const std::string &name) {
  std::vector<char> buf;
  if (parent_ent_id == no_id) {
    write_scalar(buf, record_type::root_entity_name);
    write_string(buf, name);
  } else {
    write_scalar(buf, record_type::entity_name);
    write_var_size_t(buf, parent_ent_id);
    write_string(buf, name);
  }
  io::write(fd_, buf.data(), buf.size());
}

void recorder::close() { fd_.close(); }

} // namespace update_log
} // namespace upd
