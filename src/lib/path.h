#pragma once

#include <string>
#include <vector>

namespace upd {

std::string normalize_path(const std::string& path);

std::string get_absolute_path(
  const std::string& relative_path,
  const std::string& working_path
);

struct relative_path_out_of_root_error {
  relative_path_out_of_root_error(const std::string& relative_path):
    relative_path(relative_path) {}
  const std::string relative_path;
};

std::string get_local_path(
  const std::string& root_path,
  const std::string& relative_path,
  const std::string& working_path
);

}