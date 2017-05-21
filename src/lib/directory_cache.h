#pragma once

#include "io.h"
#include <unordered_set>

namespace upd {

typedef int(*mkdir_type)(const char* pathname, mode_t mode);

struct failed_to_create_directory_error {};

/**
 * Keep track of directories that exist or not, and allows creating missing
 * directories.
 */
template <mkdir_type Mkdir>
struct directory_cache {
  /**
   * The root path is assumed pointing at an existing directory.
   */
  directory_cache(const std::string& root_path): root_path_(root_path) {}

  /**
   * The directory is created if it doesn't already exist, along with its
   * parents.
   */
  void create(const std::string& local_path) {
    if (local_path == ".") return;
    if (existing_local_paths_.count(local_path) > 0) return;
    auto local_dir_path = io::dirname_string(local_path);
    create(local_dir_path);
    auto full_path = root_path_ + '/' + local_path;
    if (Mkdir(full_path.c_str(), 0700) != 0 && errno != EEXIST) {
      throw failed_to_create_directory_error();
    }
    existing_local_paths_.insert(local_path);
  }

private:
  std::string root_path_;
  std::unordered_set<std::string> existing_local_paths_;
};

}
