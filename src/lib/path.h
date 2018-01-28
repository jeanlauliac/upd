#pragma once

#include <string>
#include <vector>

namespace upd {

/**
 * Same as `dirname`, but with `std::string`.
 */
std::string dirname(const std::string &path);

std::string normalize_path(const std::string &path);

bool is_path_absolute(const std::string &path);

std::string get_absolute_path(const std::string &relative_path,
                              const std::string &working_path);

struct relative_path_out_of_root_error {
  relative_path_out_of_root_error(const std::string &relative_path_)
      : relative_path(relative_path_) {}
  const std::string relative_path;
};

std::string get_local_path(const std::string &root_path,
                           const std::string &relative_path,
                           const std::string &working_path);

std::string get_relative_path(const std::string &target_path,
                              const std::string &relative_path,
                              const std::string &working_path);

} // namespace upd
