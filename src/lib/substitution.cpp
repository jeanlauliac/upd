#include "substitution.h"

#include <stack>

namespace upd {
namespace substitution {

void finish_segment(pattern& result, segment& current) {
  if (current.empty()) return;
  result.segments.push_back(std::move(current));
  current.clear();
}

pattern parse(const std::string& input) {
  pattern result;
  segment current;
  std::stack<size_t> capture_group_ids;
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '(') {
      finish_segment(result, current);
      capture_group_ids.push(result.capture_groups.size());
      result.capture_groups.push_back({ result.segments.size(), 0 });
      continue;
    }
    if (input[i] == ')') {
      finish_segment(result, current);
      result.capture_groups[capture_group_ids.top()].second =
        result.segments.size();
      capture_group_ids.pop();
      continue;
    }
    if (input[i] == '$') {
      finish_segment(result, current);
      ++i;
      if (i >= input.size()) throw capture_char_at_end_error();
      auto c = input[i];
      if (!(c >= '1' && c <= '9')) throw invalid_capture_index_error();
      current.has_captured_group = true;
      current.captured_group_ix = c - '1';
      continue;
    }
    if (input[i] == '\\') {
      ++i;
      if (i >= input.size()) throw escape_char_at_end_error();
    }
    current.literal += input[i];
  }
  finish_segment(result, current);
  return result;
}

resolved resolve(
  const std::vector<segment>& segments,
  const captured_string& input
) {
  resolved result;
  result.segment_start_ids.resize(segments.size());
  for (size_t i = 0; i < segments.size(); ++i) {
    result.segment_start_ids[i] = result.value.size();
    const auto& segment = segments[i];
    if (segment.has_captured_group) {
      result.value += input.get_sub_string(segment.captured_group_ix);
    }
    result.value += segment.literal;
  }
  return result;
}

captured_string capture(
  const std::vector<std::pair<size_t, size_t>>& capture_groups,
  const std::string& resolved_string,
  const std::vector<size_t>& resolved_start_segment_ids
) {
  captured_string result;
  result.value = resolved_string;
  result.captured_groups.resize(capture_groups.size());
  for (size_t j = 0; j < capture_groups.size(); ++j) {
    const auto& capture_group = capture_groups[j];
    result.captured_groups[j].first =
      capture_group.first < resolved_start_segment_ids.size()
      ? resolved_start_segment_ids[capture_group.first]
      : resolved_string.size();
    result.captured_groups[j].second =
      capture_group.second < resolved_start_segment_ids.size()
      ? resolved_start_segment_ids[capture_group.second]
      : resolved_string.size();
  }
  return result;
}

}
}
