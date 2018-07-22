#pragma once

#include "../glob.h"
#include "../inspect.h"
#include "../io/io.h"
#include "../io/utils.h"
#include "pattern.h"
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

namespace upd {
namespace path_glob {

/**
 * A match object describes a single possible result of a pattern search. For
 * example a pattern `src/(** / *).cpp` may generate a match which `local_path`
 * is `src/lib/foo.cpp`; and which single captured group is `lib/foo`
 * (described by the start and end indices instead of strings, for performance).
 */
struct match {
  std::string get_captured_string(size_t index) const {
    const auto &group = captured_groups[index];
    return local_path.substr(group.first, group.second - group.first);
  }

  size_t pattern_ix;
  std::string local_path;
  std::vector<std::pair<size_t, size_t>> captured_groups;
};

template <typename DirFilesReader> struct matcher {
private:
  struct bookmark {
    size_t pattern_ix;
    size_t segment_ix;
    std::vector<size_t> captured_from_ids;
    std::vector<size_t> captured_to_ids;
  };
  typedef std::unordered_map<std::string, std::vector<bookmark>>
      pending_dirs_type;

  enum class ent_type { unknown, regular, directory, unsupported };

public:
  matcher(const std::string &root_path, const std::vector<pattern> &patterns)
      : root_path_(root_path), patterns_(patterns),
        pending_dirs_(generate_initial_pending_dirs_(patterns)),
        bookmark_ix_(0), ent_(nullptr), ent_had_final_match_(false) {}

  matcher(const std::string &root_path, const pattern &single_pattern)
      : matcher(root_path, std::vector<pattern>({single_pattern})) {}

  bool next(match &next_match) {
    while (next_bookmark_()) {
      std::string name = ent_->d_name;
      const auto &bookmark = bookmarks_[bookmark_ix_];
      const auto &segments = patterns_[bookmark.pattern_ix].segments;
      auto segment_ix = bookmark.segment_ix;
      const auto &name_pattern = segments[segment_ix].ent_name;
      if (segments[segment_ix].has_wildcard &&
          get_type_() == ent_type::directory) {
        push_wildcard_match_(name, bookmark);
      }
      std::vector<size_t> indices;
      if (!glob::match(name_pattern, name, indices)) continue;
      if (get_type_() == ent_type::directory &&
          segment_ix + 1 < segments.size()) {
        push_ent_name_match_(name, bookmark, indices);
      }
      if (ent_had_final_match_) continue;
      if (get_type_() == ent_type::regular &&
          segment_ix == segments.size() - 1) {
        finalize_match_(next_match, name, bookmark, indices);
        return true;
      }
    }
    return false;
  }

private:
  static pending_dirs_type
  generate_initial_pending_dirs_(const std::vector<pattern> &patterns) {
    if (patterns.empty()) return pending_dirs_type();
    std::vector<bookmark> initial_bookmarks;
    for (size_t i = 0; i < patterns.size(); ++i) {
      size_t capture_group_count = patterns[i].capture_groups.size();
      initial_bookmarks.push_back(
          {i, 0, std::vector<size_t>(capture_group_count, 1),
           std::vector<size_t>(capture_group_count, 1)});
    }
    return pending_dirs_type({{"/", std::move(initial_bookmarks)}});
  }

  ent_type get_type_() {
    if (ent_type_ != ent_type::unknown) {
      return ent_type_;
    }
    ent_type_ = ent_type::unsupported;
    struct stat stbuf;
    auto ent_path = root_path_ + path_prefix_ + ent_->d_name;
    if (io::lstat(ent_path.c_str(), &stbuf) != 0) io::throw_errno();
    if (S_ISDIR(stbuf.st_mode)) {
      ent_type_ = ent_type::directory;
    } else if (S_ISREG(stbuf.st_mode)) {
      ent_type_ = ent_type::regular;
    }
    return ent_type_;
  }

  void push_wildcard_match_(const std::string &name, const bookmark &target) {
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    pending_dirs_[path_prefix_ + name + '/'].push_back(
        {target.pattern_ix, target.segment_ix, std::move(captured_from_ids),
         std::move(captured_to_ids)});
  }

  void push_ent_name_match_(const std::string &name, const bookmark &target,
                            const std::vector<size_t> match_indices) {
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    update_captures_for_ent_name_(target, match_indices, name.size(),
                                  captured_from_ids, captured_to_ids);
    pending_dirs_[path_prefix_ + name + '/'].push_back(
        {target.pattern_ix, target.segment_ix + 1, std::move(captured_from_ids),
         std::move(captured_to_ids)});
  }

  void finalize_match_(match &next_match, const std::string &name,
                       const bookmark &target,
                       const std::vector<size_t> match_indices) {
    ent_had_final_match_ = true;
    auto captured_from_ids = target.captured_from_ids;
    auto captured_to_ids = target.captured_to_ids;
    update_captures_for_ent_name_(target, match_indices, name.size(),
                                  captured_from_ids, captured_to_ids);
    next_match.local_path = path_prefix_.substr(1) + name;
    const auto &pattern_ = patterns_[target.pattern_ix];
    next_match.captured_groups.resize(pattern_.capture_groups.size());
    next_match.pattern_ix = target.pattern_ix;
    for (size_t i = 0; i < pattern_.capture_groups.size(); ++i) {
      next_match.captured_groups[i] = {
          captured_from_ids[i] - 1,
          captured_to_ids[i] - 1,
      };
    }
  }

  void update_captures_for_ent_name_(const bookmark &target,
                                     const std::vector<size_t> match_indices,
                                     size_t ent_name_size,
                                     std::vector<size_t> &captured_from_ids,
                                     std::vector<size_t> &captured_to_ids) {
    const auto &pattern_ = patterns_[target.pattern_ix];
    for (size_t i = 0; i < pattern_.capture_groups.size(); ++i) {
      const auto &group = pattern_.capture_groups[i];
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
    ent_had_final_match_ = false;
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
    ent_type_ = convert_d_type_(ent_->d_type);
    return true;
  }

  static ent_type convert_d_type_(unsigned char d_type) {
    switch (d_type) {
    case DT_UNKNOWN:
      return ent_type::unknown;
    case DT_REG:
      return ent_type::regular;
    case DT_DIR:
      return ent_type::directory;
    }
    return ent_type::unsupported;
  }

  bool next_dir_() {
    const auto &next_dir_iter = pending_dirs_.cbegin();
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
  dirent *ent_;
  ent_type ent_type_;
  bool ent_had_final_match_;
};

} // namespace path_glob
} // namespace upd
