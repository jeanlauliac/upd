#pragma once

#include "captured_string.h"
#include <string>
#include <vector>

namespace upd {
namespace substitution {

/**
 * Thrown when the escape character '\'' is found as the last character. Ex.
 * "foo/$1/bar\". The escape character needs to be followed by the literal
 * character to insert.
 */
struct escape_char_at_end_error {};

/**
 * Thrown when the placeholder character '$' is found as the last character. Ex.
 * "foo/$". It needs to be followed by the index number of the group to insert.
 */
struct placeholder_char_at_end_error {};

/**
 * Thrown when the placeholder character is followed by an invalid number.
 * Ex. "foo/$a". It needs to be an index number between 1 and 9 inclusive.
 */
struct invalid_placeholder_index_error {};

struct segment {
  /**
   * Create an empty segment, that always resolves to an empty string "".
   */
  segment(): has_placeholder(false) {}

  /**
   * Create a segment containing just a literal, that always resolves to that
   * literal. Ex. "foo".
   */
  segment(const std::string& literal):
    literal(literal), has_placeholder(false) {}

  /**
   * Create a segment containing a placeholder followed by a literal.
   * Ex. "$2.cpp", where the placeholder has the index #1 and the
   * literal is ".cpp".
   */
  segment(size_t placeholder_ix, const std::string& literal = ""):
    literal(literal),
    has_placeholder(true),
    placeholder_ix(placeholder_ix) {}

  /**
   * Set the segment to an empty literal and no capture group.
   */
  void clear() {
    literal.clear();
    has_placeholder = false;
  }

  /**
   * Check if the segment would always resolve to an empty string.
   */
  bool empty() const {
    return literal.empty() && !has_placeholder;
  }

  std::string literal;
  bool has_placeholder;
  size_t placeholder_ix;
};

inline bool operator==(const segment& left, const segment& right) {
  return
    left.literal == right.literal &&
    left.has_placeholder == right.has_placeholder && (
      !left.has_placeholder ||
      left.placeholder_ix == right.placeholder_ix
    );
}

/**
 * Represent a substitution pattern after parsing. Take, for example,
 * "src/$1/($2.cpp)". For arguments "foo" and "bar" (in that order), this
 * would resolve to "src/foo/bar.cpp", with a single captured group "bar.cpp".
 * This example pattern would be split by the parsing algorithm
 * into three `segments`:
 *
 *   [0] "src/", a simple literal;
 *   [1] "$1/", composed of the placeholder #0, and the literal "/";
 *   [2] "$2.cpp", ie. the placeholder #1 followed by literal ".cpp".
 *
 * In addition, there would be a single `capture_groups` from segment indices 2
 * to 3 (end index is non-inclusive, so that means "only the third segment"
 * given the segments above).
 */
struct pattern {
  std::vector<segment> segments;
  std::vector<std::pair<size_t, size_t>> capture_groups;
};

inline bool operator==(const pattern& left, const pattern& right) {
  return
    left.segments == right.segments &&
    left.capture_groups == right.capture_groups;
}

/**
 * Represent the result of resolving a substitution pattern, that is, replacing
 * each placeholder by actual values. `segment_start_ids` tells us the indices
 * of the characters in `value` for each segment that was resolved.
 */
struct resolved {
  std::string value;
  std::vector<size_t> segment_start_ids;
};

/**
 * Transform a string representation of a substitution pattern into a model
 * that is easy to resolve.
 */
pattern parse(const std::string& input);

resolved resolve(
  const std::vector<segment>& segments,
  const captured_string& input
);

captured_string capture(
  const std::vector<std::pair<size_t, size_t>>& capture_groups,
  const std::string& resolved_string,
  const std::vector<size_t>& resolved_start_segment_ids
);

}
}
