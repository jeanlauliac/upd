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

recorder::recorder(const std::string &file_path, record_mode mode)
    : fd_(io::open(file_path,
                   (mode == record_mode::append ? 0 : O_TRUNC) | O_WRONLY |
                       O_APPEND | O_CREAT | O_SYNC,
                   S_IRUSR | S_IWUSR)) {}

void recorder::record(const std::string &local_file_path,
                      const file_record &record) {
  std::vector<char> buf;
  write_scalar(buf, record_type::file_update);
  write_scalar(buf, record.imprint);
  write_scalar(buf, record.hash);
  write_string(buf, local_file_path);
  write_scalar(buf,
               static_cast<uint16_t>(record.dependency_local_paths.size()));
  for (const auto &dep_path : record.dependency_local_paths) {
    write_string(buf, dep_path);
  }
  io::write(fd_, buf.data(), buf.size());
}

void recorder::close() { fd_.close(); }

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
    return cache(log_file_path, records_by_file());
  }
  fd_char_reader reader(fd);
  return cache(log_file_path, read(reader));
}

void rewrite_file(const std::string &file_path,
                  const std::string &temporary_file_path,
                  const records_by_file &records) {
  recorder fresh_recorder(temporary_file_path, record_mode::truncate);
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
