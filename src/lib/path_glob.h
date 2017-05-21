#pragma once

#include "glob.h"
#include "io.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace upd {
namespace path_glob {

enum class capture_point_type { wildcard, ent_name };

/**
 * A "capture point" describe the location in a path pattern where a particular
 * capture group starts or ends. For example, in `src/(** / *).cpp`, the single
 * capture group starts from a wildcard, on the second segment, and ends inside
 * a path entity name ("ent" name).
 */
struct capture_point {
  bool is_wildcard(size_t target_segment_ix) const {
    return
      type == capture_point_type::wildcard &&
      segment_ix == target_segment_ix;
  }

  bool is_ent_name(size_t target_segment_ix) const {
    return
      type == capture_point_type::ent_name &&
      segment_ix == target_segment_ix;
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
inline bool operator==(const capture_point& left, const capture_point& right) {
  return
    left.type == right.type &&
    left.segment_ix == right.segment_ix && (
      left.type == capture_point_type::wildcard ||
      left.ent_name_segment_ix == right.ent_name_segment_ix
    );
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

inline bool operator==(const capture_group& left, const capture_group& right) {
  return
    left.from == right.from &&
    left.to == right.to;
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

inline bool operator==(const segment& left, const segment& right) {
  return
    left.has_wildcard == right.has_wildcard &&
    left.ent_name == right.ent_name;
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

inline bool operator==(const pattern& left, const pattern& right) {
  return
    left.segments == right.segments &&
    left.capture_groups == right.capture_groups;
}

enum class invalid_pattern_string_reason {
  duplicate_directory_wildcard,
  duplicate_wildcard,
  escape_char_at_end,
  unexpected_capture_close,
};

struct invalid_pattern_string_error {
  invalid_pattern_string_error(invalid_pattern_string_reason reason):
    reason(reason) {};

  invalid_pattern_string_reason reason;
};

/**
 * Parses a string, for example `src/(** / *).cpp`, into its corresponding
 * pattern object. Parsing has two goals:
 *
 *   * it checks that the pattern is valid, for example, that all the capture
 *     group parentheses are properly closed, or that wildcards are correct;
 *   * it generates a representation of the pattern that is easier and more
 *     performant for the pattern matcher to work with than a raw string, for
 *     example handling special characters escaping.
 */
pattern parse(const std::string& pattern_string);

/**
 * A match object describes a single possible result of a pattern search. For
 * example a pattern `src/(** / *).cpp` may generate a match which `local_path`
 * is `src/lib/foo.cpp`; and which single captured group is `lib/foo`
 * (described by the start and end indices instead of strings, for performance).
 */
struct match {
  std::string get_captured_string(size_t index) const {
    const auto& group = captured_groups[index];
    return local_path.substr(group.first, group.second - group.first);
  }

  size_t pattern_ix;
  std::string local_path;
  std::vector<std::pair<size_t, size_t>> captured_groups;
};

template <typename DirFilesReader>
struct matcher {
private:
  struct bookmark {
    size_t pattern_ix;
    size_t segment_ix;
    std::vector<size_t> captured_from_ids;
    std::vector<size_t> captured_to_ids;
  };
  typedef std::unordered_map<std::string, std::vector<bookmark>>
    pending_dirs_type;

public:
  matcher(const std::string& root_path, const std::vector<pattern>& patterns):
    root_path_(root_path),
    patterns_(patterns),
    pending_dirs_(generate_initial_pending_dirs_(patterns)),
    bookmark_ix_(0),
    ent_(nullptr) {}

  matcher(const std::string& root_path, const pattern& single_pattern):
    matcher(root_path, std::vector<pattern>({ single_pattern })) {}

  bool next(match& next_match) {
    while (next_bookmark_()) {
      std::string name = ent_->d_name;
      const auto& bookmark = bookmarks_[bookmark_ix_];
      const auto& segments = patterns_[bookmark.pattern_ix].segments;
      auto segment_ix = bookmark.segment_ix;
      const auto& name_pattern = segments[segment_ix].ent_name;
      if (segments[segment_ix].has_wildcard && ent_->d_type == DT_DIR) {
        push_wildcard_match_(name, bookmark);
      }
      std::vector<size_t> indices;
      if (!glob::match(name_pattern, name, indices)) continue;
      if (ent_->d_type == DT_DIR && segment_ix + 1 < segments.size()) {
        push_ent_name_match_(name, bookmark, indices);
      }
      if (ent_->d_type == DT_REG && segment_ix == segments.size() - 1) {
        finalize_match_(next_match, name, bookmark, indices);
        return true;
      }
    }
    return false;
  }

private:
  static pending_dirs_type generate_initial_pending_dirs_(
    const std::vector<pattern>& patterns
  ) {
    if (patterns.empty()) return pending_dirs_type();
    std::vector<bookmark> initial_bookmarks;
    for (size_t i = 0; i < patterns.size(); ++i) {
      size_t capture_group_count = patterns[i].capture_groups.size();
      initial_bookmarks.push_back({
        .pattern_ix = i,
        .segment_ix = 0,
        .captured_from_ids = std::vector<size_t>(capture_group_count, 1),
        .captured_to_ids = std::vector<size_t>(capture_group_count, 1),
      });
    }
    return pending_dirs_type({ { "/", std::move(initial_bookmarks) } });
  }

  void push_wildcard_match_(const std::string& name, const bookmark& target) {
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    pending_dirs_[path_prefix_ + name + '/'].push_back({
      .segment_ix = target.segment_ix,
      .captured_from_ids = std::move(captured_from_ids),
      .captured_to_ids = std::move(captured_to_ids),
      .pattern_ix = target.pattern_ix,
    });
  }

  void push_ent_name_match_(
    const std::string& name,
    const bookmark& target,
    const std::vector<size_t> match_indices
  ) {
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    update_captures_for_ent_name_(
      target,
      match_indices,
      name.size(),
      captured_from_ids,
      captured_to_ids
    );
    pending_dirs_[path_prefix_ + name + '/'].push_back({
      .segment_ix = target.segment_ix + 1,
      .captured_from_ids = std::move(captured_from_ids),
      .captured_to_ids = std::move(captured_to_ids),
      .pattern_ix = target.pattern_ix,
    });
  }

  void finalize_match_(
    match& next_match,
    const std::string& name,
    const bookmark& target,
    const std::vector<size_t> match_indices
  ) {
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    update_captures_for_ent_name_(
      target,
      match_indices,
      name.size(),
      captured_from_ids,
      captured_to_ids
    );
    next_match.local_path = path_prefix_.substr(1) + name;
    const auto& pattern_ = patterns_[target.pattern_ix];
    next_match.captured_groups.resize(pattern_.capture_groups.size());
    next_match.pattern_ix = target.pattern_ix;
    for (size_t i = 0; i < pattern_.capture_groups.size(); ++i) {
      next_match.captured_groups[i] = {
        captured_from_ids[i] - 1,
        captured_to_ids[i] - 1,
      };
    }
  }

  void update_captures_for_ent_name_(
    const bookmark& target,
    const std::vector<size_t> match_indices,
    size_t ent_name_size,
    std::vector<size_t>& captured_from_ids,
    std::vector<size_t>& captured_to_ids
  ) {
    const auto& pattern_ = patterns_[target.pattern_ix];
    for (size_t i = 0; i < pattern_.capture_groups.size(); ++i) {
      const auto& group = pattern_.capture_groups[i];
      if (group.from.is_ent_name(target.segment_ix)) {
        auto ent_name_ix = match_indices[group.from.ent_name_segment_ix];
        captured_from_ids[i] = path_prefix_.size() + ent_name_ix;
      }
      if (group.to.is_ent_name(target.segment_ix)) {
        auto ent_name_ix = group.to.ent_name_segment_ix < match_indices.size()
          ? match_indices[group.to.ent_name_segment_ix]
          : ent_name_size;
        captured_to_ids[i] = path_prefix_.size() + ent_name_ix;
      }
      if (group.from.is_wildcard(target.segment_ix + 1)) {
        captured_from_ids[i] = path_prefix_.size() + ent_name_size + 1;
      }
    }
  }

  bool next_bookmark_() {
    ++bookmark_ix_;
    if (bookmark_ix_ < bookmarks_.size()) return true;
    bookmark_ix_ = 0;
    if (!next_ent_()) return false;
    while (ent_->d_name[0] == '.') {
      if (!next_ent_()) return false;
    }
    return true;
  }

  bool next_ent_() {
    if (!dir_reader_.is_open()) {
      if (!next_dir_()) return false;
    }
    ent_ = dir_reader_.next();
    while (ent_ == nullptr) {
      if (!next_dir_()) {
        dir_reader_.close();
        return false;
      }
      ent_ = dir_reader_.next();
    }
    return true;
  }

  bool next_dir_() {
    const auto& next_dir_iter = pending_dirs_.cbegin();
    if (next_dir_iter == pending_dirs_.cend()) {
      return false;
    }
    path_prefix_ = next_dir_iter->first;
    bookmarks_ = next_dir_iter->second;
    pending_dirs_.erase(next_dir_iter);
    dir_reader_.open(root_path_ + path_prefix_);
    return true;
  }

  std::string root_path_;
  std::vector<pattern> patterns_;
  DirFilesReader dir_reader_;
  pending_dirs_type pending_dirs_;
  std::string path_prefix_;
  std::vector<bookmark> bookmarks_;
  size_t bookmark_ix_;
  dirent* ent_;
};

}
}
