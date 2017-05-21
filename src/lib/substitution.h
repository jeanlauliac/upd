#pragma once

#include "captured_string.h"
#include <string>
#include <vector>

namespace upd {
namespace substitution {

struct escape_char_at_end_error {};
struct capture_char_at_end_error {};
struct invalid_capture_index_error {};

struct segment {
  segment(): has_captured_group(false) {}
  segment(const std::string& literal):
    literal(literal), has_captured_group(false) {}
  segment(size_t captured_group_ix, const std::string& literal = ""):
    literal(literal),
    has_captured_group(true),
    captured_group_ix(captured_group_ix) {}

  void clear() {
    literal.clear();
    has_captured_group = false;
  }

  bool empty() const {
    return literal.empty() && !has_captured_group;
  }

  std::string literal;
  bool has_captured_group;
  size_t captured_group_ix;
};

inline bool operator==(const segment& left, const segment& right) {
  return
    left.literal == right.literal &&
    left.has_captured_group == right.has_captured_group && (
      !left.has_captured_group ||
      left.captured_group_ix == right.captured_group_ix
    );
}

struct pattern {
  std::vector<segment> segments;
  std::vector<std::pair<size_t, size_t>> capture_groups;
};

inline bool operator==(const pattern& left, const pattern& right) {
  return
    left.segments == right.segments &&
    left.capture_groups == right.capture_groups;
}

struct resolved {
  std::string value;
  std::vector<size_t> segment_start_ids;
};

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
