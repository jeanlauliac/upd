#pragma once

#include <vector>

namespace upd {
namespace update_log {

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

} // namespace update_log
} // namespace upd
