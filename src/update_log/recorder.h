#pragma once

#include "../../gen/src/update_log/file_record.h"
#include "../io/file_descriptor.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace upd {
namespace update_log {

constexpr char VERSION = 2;

typedef std::unordered_map<std::string, uint16_t> ent_ids_by_path;
typedef std::vector<std::string> string_vector;

/**
 * Allow us to write to the update log as we go (normally ".upd/log"). We append
 * records to the update log as soon as we updated a particular file. This
 * allows us to support crashes out-of-the-box, with no specific exit code.
 * For example, if we update file A, but then the process receive SIGINT, or it
 * throws an exception, we already persisted the information about A.
 */
struct recorder {
  recorder(const std::string &file_path);
  recorder(const std::string &file_path, const string_vector &ent_paths);
  void record(const std::string &local_file_path, const file_record &record);
  void close();

private:
  uint16_t get_path_id_(const std::string &file_path);
  void record_ent_name_(uint16_t parent_ent_id, const std::string &name);

  io::file_descriptor fd_;
  ent_ids_by_path ent_ids_by_path_;
};

} // namespace update_log
} // namespace upd
