#pragma once

#include "../file_descriptor.h"
#include "file_record.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace upd {
namespace update_log {

enum class record_mode { append, truncate };

/**
 * Allow us to write to the update log as we go (normally ".upd/log"). We append
 * records to the update log as soon as we updated a particular file. This
 * allows us to support crashes out-of-the-box, with no specific exit code.
 * For example, if we update file A, but then the process receive SIGINT, or it
 * throws an exception, we already persisted the information about A.
 */
struct recorder {
  recorder(const std::string &file_path, record_mode mode);
  void record(const std::string &local_file_path, const file_record &record);
  void close();

private:
  uint16_t get_path_id_(const std::string &file_path);
  void record_ent_name_(uint16_t parent_ent_id, const std::string &name);

  file_descriptor fd_;
  std::unordered_map<std::string, uint16_t> ent_ids_by_path_;
};

typedef std::unordered_map<std::string, file_record> records_by_file;

/**
 * We keep of copy of the update log in memory. New elements added to the
 * cache are persisted right away (see `recorder`).
 */
struct cache {
  cache(const std::string &file_path, const records_by_file &cached_records);
  records_by_file::iterator find(const std::string &local_file_path);
  records_by_file::iterator end();
  void record(const std::string &local_file_path, const file_record &record);
  void close() { recorder_.close(); }
  const records_by_file &records() const { return cached_records_; }
  static records_by_file
  records_from_log_file(const std::string &log_file_path);
  static cache from_log_file(const std::string &log_file_path);

private:
  recorder recorder_;
  records_by_file cached_records_;
};

struct failed_to_rewrite_error {};

/**
 * Once all the updates have happened, we have appended new entries to the
 * update log, but there may be duplicates. To finalize the log, we rewrite it
 * from scratch, and we replace the existing log using a file rename. `rename`
 * is normally an atomic operation, so we ensure no data is lost even if the
 * process crashes right in the middle of the rewrite.
 */
void rewrite_file(const std::string &file_path,
                  const std::string &temporary_file_path,
                  const records_by_file &records);

} // namespace update_log
} // namespace upd
