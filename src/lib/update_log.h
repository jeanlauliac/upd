#pragma once

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace upd {
namespace update_log {

/**
 * It means the update log has been corrupted.
 */
struct corruption_error {};

/**
 * For each file we update, we keep track of how we generated it, and what
 * we got. This allows us, the next time around, to know if a file needs
 * update again, or is up-to-date.
 */
struct file_record {
  /**
   * This is a hash digest of the command line that generated that particular
   * file, plus all the source files and dependencies content. When we wish to
   * update a file, we recompute this hash from the fresh command line; if it's
   * the same as the record, it means it's already up-to-date.
   */
  unsigned long long imprint;
  /**
   * This is a hash digest of the file content. This is handy to know if a file
   * hasn't been corrupted after having been generated; in which case it needs
   * to be updated again.
   */
  unsigned long long hash;
  /**
   * This is all the files on which this particular file depends on for it
   * generation, in addition to its direct sources. For example, in C++,
   * an object file depends on the `.cpp` file, but headers are additional
   * dependencies that we must consider.
   *
   * Another example: if we use a JavaScript script to update some files,
   * this script itself has modules it depends on. If these modules change,
   * it's probably best to update the files again.
   */
  std::vector<std::string> dependency_local_paths;
};

enum class record_mode { append, truncate };

/**
 * Allow us to write to the update log as we go (normally ".upd/log"). We append
 * records to the update log as soon as we updated a particular file. This
 * allows us to support crashes out-of-the-box, with no specific exit code.
 * For example, if we update file A, but then the process receive SIGINT, or it
 * throws an exception, we already persisted the information about A.
 */
struct recorder {
  recorder(const std::string& file_path, record_mode mode);
  void record(const std::string& local_file_path, const file_record& record);
  void close();

private:
  std::ofstream log_file_;
};

typedef std::unordered_map<std::string, file_record> records_by_file;

/**
 * We keep of copy of the update log in memory. New elements added to the
 * cache are persisted right away (see `recorder`).
 */
struct cache {
  cache(const std::string& file_path, const records_by_file& cached_records):
    recorder_(file_path, record_mode::append), cached_records_(cached_records) {}
  records_by_file::iterator find(const std::string& local_file_path);
  records_by_file::iterator end();
  void record(const std::string& local_file_path, const file_record& record);
  void close() { recorder_.close(); }
  const records_by_file& records() const { return cached_records_; }
  static records_by_file records_from_log_file(const std::string& log_file_path);
  static cache from_log_file(const std::string& log_file_path);

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
void rewrite_file(
  const std::string& file_path,
  const std::string& temporary_file_path,
  const records_by_file& records
);

}
}
