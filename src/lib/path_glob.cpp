#include "path_glob.h"
#include <stack>

namespace upd {
namespace path_glob {

struct pattern_string_parser {
  pattern_string_parser(const std::string& input):
    input(input) {}

  pattern operator()() {
    input_ix = 0;
    result.segments.clear();
    current_segment.clear();
    read_path_segment();
    while (!current_segment.ent_name.empty()) {
      result.segments.push_back(current_segment);
      read_path_segment();
    }
    return result;
  }

  void read_path_segment() {
    current_segment.ent_name.clear();
    current_segment.has_wildcard = read_directory_wildcard();
    if (read_directory_wildcard()) {
      throw invalid_pattern_string_error(invalid_pattern_string_reason::duplicate_directory_wildcard);
    }
    while (input_ix < input.size() && input[input_ix] != '/') {
      process_input_char();
      ++input_ix;
    }
    finish_glob_segment();
    if (input_ix < input.size()) ++input_ix;
  }

  void process_input_char() {
    if (input[input_ix] == '*') {
      if (
        current_glob_segment.prefix == upd::glob::placeholder::wildcard &&
        current_glob_segment.literal.empty()
      ) {
        throw invalid_pattern_string_error(invalid_pattern_string_reason::duplicate_wildcard);
      }
      finish_glob_segment();
      current_glob_segment.prefix = upd::glob::placeholder::wildcard;
      return;
    }
    if (input[input_ix] == '?') {
      finish_glob_segment();
      current_glob_segment.prefix = upd::glob::placeholder::single_wildcard;
      return;
    }
    if (input[input_ix] == '(') {
      open_capture_group_in_name();
      return;
    }
    if (input[input_ix] == ')') {
      close_capture_group();
      return;
    }
    if (input[input_ix] == '\\') ++input_ix;
    if (input_ix == input.size()) {
      throw invalid_pattern_string_error(invalid_pattern_string_reason::escape_char_at_end);
    }
    current_glob_segment.literal += input[input_ix];
  }

  void open_capture_group_in_name() {
    finish_glob_segment();
    open_capture_group({
      .type = capture_point_type::ent_name,
      .ent_name_segment_ix = current_segment.ent_name.size(),
      .segment_ix = result.segments.size(),
    });
  }

  void close_capture_group() {
    finish_glob_segment();
    if (capture_groups_ids.empty()) {
      throw invalid_pattern_string_error(invalid_pattern_string_reason::unexpected_capture_close);
    }
    auto& group = result.capture_groups[capture_groups_ids.top()];
    capture_groups_ids.pop();
    group.to = {
      .type = capture_point_type::ent_name,
      .ent_name_segment_ix = current_segment.ent_name.size(),
      .segment_ix = result.segments.size(),
    };
  }

  void finish_glob_segment() {
    if (current_glob_segment.empty()) return;
    current_segment.ent_name.push_back(std::move(current_glob_segment));
    current_glob_segment.clear();
  }

  bool read_directory_wildcard() {
    bool open_capture = false, close_capture = false;
    size_t ix = input_ix;
    if (ix >= input.size()) return false;
    if (input[ix] == '(') {
      open_capture = true;
      ++ix;
    }
    if (ix + 2 >= input.size()) return false;
    if (input[ix] != '*' || input[ix + 1] != '*') return false;
    ix += 2;
    if (ix + 1 >= input.size()) return false;
    if (input[ix] == ')') {
      close_capture = true;
      ++ix;
    }
    if (ix >= input.size()) return false;
    if (input[ix] != '/') return false;
    input_ix = ix + 1;
    if (open_capture) {
      open_capture_group({
        .type = capture_point_type::wildcard,
        .segment_ix = result.segments.size(),
      });
    }
    if (close_capture) {
      throw std::runtime_error("not implemented!");
    }
    return true;
  }

  void open_capture_group(const capture_point& from) {
    result.capture_groups.push_back(capture_group({ .from = from }));
    capture_groups_ids.push(result.capture_groups.size() - 1);
  }

  const std::string& input;
  size_t input_ix;
  segment current_segment;
  upd::glob::segment current_glob_segment;
  pattern result;
  std::stack<size_t> capture_groups_ids;
};

pattern parse(const std::string& pattern_string) {
  return pattern_string_parser(pattern_string)();
}

}
}
