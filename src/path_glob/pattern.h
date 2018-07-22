#pragma once

#include "../glob.h"
#include "../inspect.h"
#include <string>
#include <vector>

namespace upd {
namespace path_glob {

enum class capture_point_type { wildcard, ent_name };

inline std::string inspect(capture_point_type value,
                           const inspect_options &options) {
  return inspect(static_cast<size_t>(value), options);
}

/**
 * A "capture point" describe the location in a path pattern where a particular
 * capture group starts or ends. For example, in `src/(** / *).cpp`, the single
 * capture group starts from a wildcard, on the second segment, and ends inside
 * a path entity name ("ent" name).
 */
struct capture_point {
  bool is_wildcard(size_t target_segment_ix) const {
    return type == capture_point_type::wildcard &&
           segment_ix == target_segment_ix;
  }

  bool is_ent_name(size_t target_segment_ix) const {
    return type == capture_point_type::ent_name &&
           segment_ix == target_segment_ix;
  }

  static capture_point wildcard(size_t segment_ix) {
    return capture_point{segment_ix, capture_point_type::wildcard, 0};
  }

  /**
   * Index of the pattern segment, described by the pattern object provided
   * separately. The pattern parser must ensure that this is always pointing
   * at an existing segment.
   */
  size_t segment_ix;

  /**
   * The type of the path part we want to capture, either a wildcard or an
   * entity name. If we're capturing a wildcard, its entire match will be
   * included, whether it starts or end the capture. If it's an entity name,
   * the `ent_name_segment_ix` decribes the further sub-division to be included.
   */
  capture_point_type type;

  /**
   * This field is exclusively valid for a capture point of type `ent_name`.
   * It describe where the capture should start or end inside the entity
   * name itself. For example, for `(src/ *).cpp` the capture ends at the
   * second ent-name sub-segment (the period), of the second pattern segment
   * (`*.cpp`).
   */
  size_t ent_name_segment_ix;
};

/**
 * `ent_name_segment_ix` is only valid for a capture point of type `ent-name`,
 * that explains why we ignore it otherwise.
 */
inline bool operator==(const capture_point &left, const capture_point &right) {
  return left.type == right.type && left.segment_ix == right.segment_ix &&
         (left.type == capture_point_type::wildcard ||
          left.ent_name_segment_ix == right.ent_name_segment_ix);
}

inline std::string inspect(const capture_point &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::path_glob::capture_group", options);
  insp.push_back("segment_ix", value.segment_ix);
  insp.push_back("type", value.type);
  insp.push_back("ent_name_segment_ix", value.ent_name_segment_ix);
  return insp.result();
}

/**
 * A capture group describes where a capture should start end end within a path
 * pattern. For example, in `src/(** / *).cpp` there is a single capture group
 * that starts by the folder wildcard, and ends at the extension period of the
 * file name.
 */
struct capture_group {
  capture_point from;
  capture_point to;
};

inline bool operator==(const capture_group &left, const capture_group &right) {
  return left.from == right.from && left.to == right.to;
}

inline std::string inspect(const capture_group &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::path_glob::capture_group", options);
  insp.push_back("from", value.from);
  insp.push_back("to", value.to);
  return insp.result();
}

/**
 * A path pattern segment is the most basic unit that we are using to match a
 * file path. These are computed by parsing a pattern string. For example, the
 * pattern string `src/ ** / *.cpp` will output two segments. The first segment
 * is `src` and describe a simple path entity name, no wildcard. The second
 * segment is `** / *.cpp`. This one is a bit more complex: it describes both
 * an entity name, `*.cpp`, and a prefix wildcard.
 */
struct segment {
  void clear() {
    ent_name.clear();
    has_wildcard = false;
  }

  /**
   * The entity name we're trying to match. For example `*.cpp`, that would
   * match all the C++ files of a particular directory.
   */
  glob::pattern ent_name;

  /**
   * If `true`, then we're trying to match a folder wildcard. This field is the
   * only difference between, for example, `** / *.cpp`, and `*.cpp`: they have
   * the same entity name pattern, but one has an additional prefix wildcard. It
   * means when we're trying to match this segment, we'll both process folders,
   * always matching the wildcard, and names matching the entity name pattern.
   * An entity (file or folder) is allowed to match both the wildcard and the
   * entity name pattern.
   */
  bool has_wildcard;
};

inline bool operator==(const segment &left, const segment &right) {
  return left.has_wildcard == right.has_wildcard &&
         left.ent_name == right.ent_name;
}

inline std::string inspect(const segment &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::path_glob::segment", options);
  insp.push_back("ent_name", value.ent_name);
  insp.push_back("has_wildcard", value.has_wildcard);
  return insp.result();
}

/**
 * A pattern describes how a complete file path should look like, along with
 * sub-sequences we'd like to keep note of, called "capture groups". A pattern
 * object is generated as the output of parsing a pattern string. For example,
 * the pattern string `src/(** / *).cpp` would result in a pattern object that
 * has two segments and a single capture group.
 */
struct pattern {
  std::vector<capture_group> capture_groups;
  std::vector<segment> segments;
};

inline bool operator==(const pattern &left, const pattern &right) {
  return left.segments == right.segments &&
         left.capture_groups == right.capture_groups;
}

inline std::string inspect(const pattern &value,
                           const inspect_options &options) {
  collection_inspector insp("upd::path_glob::pattern", options);
  insp.push_back("capture_groups", value.capture_groups);
  insp.push_back("segments", value.segments);
  return insp.result();
}

enum class invalid_pattern_string_reason {
  duplicate_directory_wildcard,
  duplicate_wildcard,
  escape_char_at_end,
  unexpected_capture_close,
};

struct invalid_pattern_string_error {
  invalid_pattern_string_error(invalid_pattern_string_reason reason_)
      : reason(reason_) {}

  invalid_pattern_string_reason reason;
};

} // namespace path_glob

template <> struct type_info<path_glob::pattern> {
  static const char *name() { return "upd::path_glob::pattern"; }
};
template <> struct type_info<path_glob::segment> {
  static const char *name() { return "upd::path_glob::segment"; }
};
template <> struct type_info<path_glob::capture_group> {
  static const char *name() { return "upd::path_glob::capture_group"; }
};

} // namespace upd
