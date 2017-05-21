#include "io.h"
#include "update_log.h"
#include <iomanip>
#include <sstream>

namespace upd {
namespace update_log {

recorder::recorder(const std::string& file_path, record_mode mode) {
  log_file_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  log_file_.open(file_path, mode == record_mode::append ? std::ios::app : 0);
  log_file_ << std::setfill('0') << std::setw(16) << std::hex;
}

void recorder::record(const std::string& local_file_path, const file_record& record) {
  upd::io::stream_string_joiner<std::ofstream> joiner(log_file_, " ");
  joiner.push(record.hash).push(record.imprint).push(local_file_path);
  for (auto path: record.dependency_local_paths) joiner.push(path);
  log_file_ << std::endl;
}

void recorder::close() {
  log_file_.close();
}

records_by_file::iterator cache::find(const std::string& local_file_path) {
  return cached_records_.find(local_file_path);
}

records_by_file::iterator cache::end() {
  return cached_records_.end();
}

void cache::record(const std::string& local_file_path, const file_record& record) {
  recorder_.record(local_file_path, record);
  cached_records_[local_file_path] = record;
}

records_by_file cache::records_from_log_file(const std::string& log_file_path) {
  records_by_file records;
  std::ifstream log_file;
  log_file.exceptions(std::ifstream::badbit);
  log_file.open(log_file_path);
  std::string line, local_file_path, dep_file_path;
  std::istringstream iss;
  file_record record_being_read;
  while (std::getline(log_file, line)) {
    record_being_read.dependency_local_paths.clear();
    iss.str(line);
    iss.clear();
    iss >> std::hex >> record_being_read.hash
        >> record_being_read.imprint >> local_file_path;
    while (iss.good()) {
      iss >> dep_file_path;
      record_being_read.dependency_local_paths.push_back(dep_file_path);
    }
    if (iss.fail()) {
      throw corruption_error();
    }
    records[local_file_path] = record_being_read;
  }
  return records;
}

cache cache::from_log_file(const std::string& log_file_path) {
  return cache(log_file_path, records_from_log_file(log_file_path));
}

void rewrite_file(
  const std::string& file_path,
  const std::string& temporary_file_path,
  const records_by_file& records
) {
  recorder fresh_recorder(temporary_file_path, record_mode::truncate);
  for (auto record_entry: records) {
    fresh_recorder.record(record_entry.first, record_entry.second);
  }
  fresh_recorder.close();
  if (rename(temporary_file_path.c_str(), file_path.c_str()) != 0) {
    throw failed_to_rewrite_error();
  }
}

}
}
