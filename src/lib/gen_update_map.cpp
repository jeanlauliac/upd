#include "gen_update_map.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace upd {

std::vector<std::vector<captured_string>>
crawl_source_patterns(const std::string &root_path,
                      const std::vector<path_glob::pattern> &patterns) {
  std::vector<std::vector<captured_string>> matches(patterns.size());
  path_glob::matcher<io::dir_files_reader> matcher(root_path, patterns);
  path_glob::match match;
  while (matcher.next(match)) {
    matches[match.pattern_ix].push_back({
        .value = std::move(match.local_path),
        .captured_groups = std::move(match.captured_groups),
    });
  }
  for (size_t i = 0; i < matches.size(); ++i) {
    auto &fileMatches = matches[i];
    if (fileMatches.empty()) throw no_source_matches_error(i);
    std::sort(fileMatches.begin(), fileMatches.end());
  }
  return matches;
}

update_map gen_update_map(const std::string &root_path,
                          const manifest::manifest &manifest) {
  update_map result;
  auto matches = crawl_source_patterns(root_path, manifest.source_patterns);
  std::vector<std::vector<captured_string>> rule_captured_paths(
      manifest.rules.size());
  std::unordered_map<std::string, size_t> rule_ids_by_output_path;
  for (size_t i = 0; i < manifest.rules.size(); ++i) {
    const auto &rule = manifest.rules[i];
    std::unordered_map<std::string,
                       std::pair<std::vector<std::string>, std::vector<size_t>>>
        data_by_path;
    for (const auto &input : rule.inputs) {
      if (input.type == manifest::update_rule_input::type::rule) {
        if (input.input_ix >= i) {
          throw cannot_refer_to_later_rule_error();
        }
      }
      const auto &input_captures =
          input.type == manifest::update_rule_input::type::source
              ? matches[input.input_ix]
              : rule_captured_paths[input.input_ix];
      for (const auto &input_capture : input_captures) {
        auto local_output =
            substitution::resolve(rule.output.segments, input_capture);
        auto &datum = data_by_path[local_output.value];
        datum.first.push_back(input_capture.value);
        datum.second = local_output.segment_start_ids;
      }
    }
    std::unordered_set<std::string> all_dependencies;
    for (const auto &dependency : rule.dependencies) {
      if (dependency.type == manifest::update_rule_input::type::rule) {
        if (dependency.input_ix >= i) {
          throw cannot_refer_to_later_rule_error();
        }
      }
      const auto &input_captures =
          dependency.type == manifest::update_rule_input::type::source
              ? matches[dependency.input_ix]
              : rule_captured_paths[dependency.input_ix];
      for (const auto &input_capture : input_captures) {
        all_dependencies.insert(input_capture.value);
      }
    }
    auto &captured_paths = rule_captured_paths[i];
    captured_paths.resize(data_by_path.size());
    size_t k = 0;
    for (const auto &datum : data_by_path) {
      if (result.output_files_by_path.count(datum.first)) {
        throw duplicate_output_error{
            .local_output_file_path = datum.first,
            .rule_ids = {rule_ids_by_output_path.at(datum.first), i},
        };
      }
      result.output_files_by_path[datum.first] = {
          .command_line_ix = rule.command_line_ix,
          .local_input_file_paths = datum.second.first,
          .local_dependency_file_paths = all_dependencies,
      };
      rule_ids_by_output_path[datum.first] = i;
      captured_paths[k] = substitution::capture(
          rule.output.capture_groups, datum.first, datum.second.second);
      ++k;
    }
  }
  return result;
}

} // namespace upd
