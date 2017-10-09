#include "path.h"
#include <sstream>

namespace upd {

std::vector<std::string> split_path(const std::string& path) {
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
  return parts;
}

std::string join_path(const std::vector<std::string>& parts) {
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

std::string normalize_path(const std::string& path) {
  return join_path(split_path(path));
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

std::string get_relative_path(
  const std::string& target_path,
  const std::string& relative_path,
  const std::string& working_path
) {
  auto absolute_path = get_absolute_path(relative_path, working_path);
  auto target_parts = split_path(target_path);
  auto source_parts = split_path(absolute_path);
  size_t i = 0;
  while (
    i < target_parts.size() &&
    i < source_parts.size() &&
    target_parts[i] == source_parts[i]
  ) ++i;
  std::vector<std::string> result_parts;
  for (size_t j = i; j < target_parts.size(); ++j) {
    result_parts.push_back("..");
  }
  for (size_t j = i; j < source_parts.size(); ++j) {
    result_parts.push_back(source_parts[j]);
  }
  return join_path(result_parts);
}

}
