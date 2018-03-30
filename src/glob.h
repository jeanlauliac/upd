#pragma once

#include "inspect.h"
#include "io/io.h"
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

namespace upd {
namespace glob {

enum class placeholder {
  none,
  wildcard,
  single_wildcard,
};

struct segment {
  segment() : prefix(placeholder::none) {}
  segment(placeholder prefix_) : prefix(prefix_) {}
  segment(const std::string &literal_)
      : prefix(placeholder::none), literal(literal_) {}
  segment(placeholder prefix_, const std::string &literal_)
      : prefix(prefix_), literal(literal_) {}

  void clear() {
    prefix = placeholder::none;
    literal.clear();
  }

  bool empty() const {
    return literal.empty() && prefix == upd::glob::placeholder::none;
  }

  placeholder prefix;
  std::string literal;
};

inline bool operator==(const segment &left, const segment &right) {
  return left.prefix == right.prefix && left.literal == right.literal;
}

inline std::string inspect(const segment &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::path_glob::segment", options);
  insp.push_back("prefix", value.prefix);
  insp.push_back("literal", value.literal);
  return insp.result();
}

/**
 * A pattern is composed of literals prefixed by special placeholders. For
 * example the pattern "foo_*.cpp" is represented as the following vector:
 *
 *     {
 *       { placeholder::none, "foo_" },
 *       { placeholder::wildcard, ".cpp" },
 *     }
 *
 */
typedef std::vector<segment> pattern;

bool match(const pattern &target, const std::string &candidate);

/**
 * If it matches, the `indices` vector has the same size as the number of
 * segments, and it indicates the starting index for each segment that was
 * correctly matched.
 *
 * For example `foo*.cpp` has 2 segments, `foo` and `*.cpp`. If it matches
 * `foobar.cpp`, then the indices will be `{ 0, 3 }`.
 */
bool match(const pattern &target, const std::string &candidate,
           std::vector<size_t> &indices);

} // namespace glob
} // namespace upd
