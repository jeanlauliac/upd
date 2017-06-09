#pragma once

#include <string>
#include <vector>

namespace upd {

struct no_such_captured_group_error {};

/**
 * Represent a string with "captured groups". A group is a substring of that
 * string. Such a structure is generated as the result of, for example,
 * globbing pattern. Consider `(foo*).cpp`. This has a single capture group.
 * If it matches a file name such as `foobar.cpp`, then the result is a
 * captured string `foobar.cpp` which single captured group is from character
 * zero to 6, that is, `foobar`.
 */
struct captured_string {
  /**
   * Extract a captured group as an actual string, for further processing.
   */
  std::string get_sub_string(size_t index) const {
    if (index >= captured_groups.size()) throw no_such_captured_group_error();
    const auto& group = captured_groups[index];
    return value.substr(group.first, group.second - group.first);
  }

  /**
   * Start and end indices of a group.
   */
  typedef std::pair<size_t, size_t> group;
  typedef std::vector<group> groups;

  std::string value;
  groups captured_groups;
};

/**
 * This allows us to readily sort a vector of captured strings.
 */
inline
bool operator<(const captured_string& left, const captured_string& right) {
  if (left.value < right.value) return true;
  if (left.value > right.value) return false;
  return left.captured_groups < right.captured_groups;
}

}
