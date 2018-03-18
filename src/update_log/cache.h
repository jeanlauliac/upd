#pragma once

#include "../../gen/src/update_log/file_record.h"
#include "../file_descriptor.h"
#include "recorder.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace upd {
namespace update_log {

typedef std::unordered_map<std::string, file_record> records_by_file;
struct cache_file_data {
  records_by_file records;
  string_vector ent_paths;
};

/**
 * We keep of copy of the update log in memory. New elements added to the
 * cache are persisted right away (see `recorder`).
 */
struct cache {
  cache(const std::string &file_path, const cache_file_data &data);
  cache(const std::string &file_path);
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
