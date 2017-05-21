#include "path.h"
#include <sstream>

namespace upd {

std::string normalize_path(const std::string& path) {
  std::vector<std::string> parts;
  int i = 0;
  while (i < path.size()) {
    int j = i;
    while (j < path.size() && path[j] != '/') ++j;
    auto part = path.substr(i, j - i);
    if (part == ".." && !parts.empty()) {
      parts.pop_back();
    } else if (part != ".") {
      parts.push_back(part);
    }
    while (j < path.size() && path[j] == '/') ++j;
    i = j;
  }
  if (parts.empty()) {
    return ".";
  }
  std::ostringstream oss;
  bool first = true;
  for (auto const& part: parts) {
    if (!first) oss << "/";
    first = false;
    oss << part;
  }
  auto result = oss.str();
  return result.empty() ? "/" : result;
}

std::string get_absolute_path(
  const std::string& relative_path,
  const std::string& working_path
) {
  if (relative_path.at(0) == '/') {
    return normalize_path(relative_path);
  }
  return normalize_path(working_path + '/' + relative_path);
}

std::string get_local_path(
  const std::string& root_path,
  const std::string& relative_path,
  const std::string& working_path
) {
  auto absolute_path = get_absolute_path(relative_path, working_path);
  if (absolute_path.compare(0, root_path.size(), root_path) != 0) {
    throw relative_path_out_of_root_error(relative_path);
  }
  return absolute_path.substr(root_path.size() + 1);
}

}
