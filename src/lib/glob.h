#pragma once

#include "io.h"
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
  segment(): prefix(placeholder::none) {}
  segment(placeholder prefix): prefix(prefix) {}
  segment(const std::string& literal):
    prefix(placeholder::none), literal(literal) {}
  segment(placeholder prefix, const std::string& literal):
    prefix(prefix), literal(literal) {}

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

inline bool operator==(const segment& left, const segment& right) {
  return left.prefix == right.prefix && left.literal == right.literal;
}

/**
 * A pattern is composed of literals prefixed by special placeholders. For
 * example the pattern "foo_*.cpp" is represented as the following vector:
 *
 *     {
 *       { .prefix = placeholder::none, literal. = "foo_" },
 *       { .prefix = placeholder::wildcard, ".cpp" },
 *     }
 *
 */
typedef std::vector<segment> pattern;

bool match(const pattern& target, const std::string& candidate);

/**
 * If it matches, the `indices` vector has the same size as the number of
 * segments, and it indicates the starting index for each segment that was
 * correctly matched.
 */
bool match(
  const pattern& target,
  const std::string& candidate,
  std::vector<size_t>& indices
);

}
}
