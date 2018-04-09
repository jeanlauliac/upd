#include "cache.h"
#include "../fd_char_reader.h"
#include "../io/io.h"
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

cache::cache(const std::string &file_path, const cache_file_data &data)
    : recorder_(file_path, data.ent_paths), cached_records_(data.records) {}

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
  io::file_descriptor fd;
  try {
    fd = io::open(log_file_path, O_RDONLY, 0);
  } catch (const std::system_error &error) {
    if (error.code() != std::errc::no_such_file_or_directory) throw;
    return cache(log_file_path);
  }
  fd_char_reader reader(fd);
  try {
    return cache(log_file_path, read(reader));
  } catch (const version_mismatch_error &) {
    return cache(log_file_path);
  }
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
