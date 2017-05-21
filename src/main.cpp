#include "lib/cli.h"
#include "lib/command_line_template.h"
#include "lib/depfile.h"
#include "lib/directory_cache.h"
#include "lib/inspect.h"
#include "lib/io.h"
#include "lib/istream_char_reader.h"
#include "lib/json/lexer.h"
#include "lib/manifest.h"
#include "lib/path.h"
#include "lib/path_glob.h"
#include "lib/substitution.h"
#include "lib/update.h"
#include "lib/update_log.h"
#include "lib/xxhash64.h"
#include "package.h"
#include <array>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <map>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace upd {

static const std::string CACHE_FOLDER = ".upd";

struct unknown_target_error {
  unknown_target_error(const std::string& relative_path):
    relative_path(relative_path) {}
  std::string relative_path;
};

/**
 * At any point during the update, the plan describes the work left to do.
 */
struct update_plan {
  /**
   * Remove a file from the plan. This potientially allows descendants to be
   * available for update.
   */
  void erase(const std::string& local_target_path) {
    pending_output_file_paths.erase(local_target_path);
    auto descendants_iter = descendants_by_path.find(local_target_path);
    if (descendants_iter == descendants_by_path.end()) {
      return;
    }
    for (auto const& descendant_path: descendants_iter->second) {
      auto count_iter = pending_input_counts_by_path.find(descendant_path);
      if (count_iter == pending_input_counts_by_path.end()) {
        throw std::runtime_error("update plan is corrupted");
      }
      int& input_count = count_iter->second;
      --input_count;
      if (input_count == 0) {
        queued_output_file_paths.push(descendant_path);
      }
    }
  }

  /**
   * The paths of all the output files that are ready to be updated immediately.
   * These files' dependencies either have already been updated, or they are
   * source files written manually.
   */
  std::queue<std::string> queued_output_file_paths;
  /**
   * All the files that remain to update.
   */
  std::unordered_set<std::string> pending_output_file_paths;
  /**
   * For each output file path, indicates how many input files still need to be
   * updated before the output file can be updated.
   */
  std::unordered_map<std::string, int> pending_input_counts_by_path;
  /**
   * For each input file path, indicates what files could potentially be
   * updated after the input file is updated.
   */
  std::unordered_map<std::string, std::vector<std::string>> descendants_by_path;
};

void build_update_plan(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::pair<std::string, output_file>& target_descriptor
);

bool build_update_plan_for_path(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::string& local_target_path,
  const std::string& local_input_path
) {
  auto input_descriptor = output_files_by_path.find(local_input_path);
  if (input_descriptor == output_files_by_path.end()) return false;
  plan.descendants_by_path[local_input_path].push_back(local_target_path);
  build_update_plan(plan, output_files_by_path, *input_descriptor);
  return true;
}

void build_update_plan(
  update_plan& plan,
  const std::unordered_map<std::string, output_file>& output_files_by_path,
  const std::pair<std::string, output_file>& target_descriptor
) {
  auto local_target_path = target_descriptor.first;
  auto pending = plan.pending_output_file_paths.find(local_target_path);
  if (pending != plan.pending_output_file_paths.end()) {
    return;
  }
  plan.pending_output_file_paths.insert(local_target_path);
  int input_count = 0;
  for (auto const& local_input_path: target_descriptor.second.local_input_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path, local_target_path, local_input_path)) {
      input_count++;
    }
  }
  for (auto const& local_dependency_path: target_descriptor.second.local_dependency_file_paths) {
    if (build_update_plan_for_path(plan, output_files_by_path, local_target_path, local_dependency_path)) {
      input_count++;
    }
  }
  if (input_count == 0) {
    plan.queued_output_file_paths.push(local_target_path);
  } else {
    plan.pending_input_counts_by_path[local_target_path] = input_count;
  }
}

void execute_update_plan(
  update_log::cache& log_cache,
  file_hash_cache& hash_cache,
  const std::string& root_path,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates,
  const std::string& local_depfile_path,
  bool print_commands,
  directory_cache<mkdir>& dir_cache
) {
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);
    auto target_file = target_descriptor.second;
    auto const& command_line_tpl = command_line_templates[target_file.command_line_ix];
    update_file(
      log_cache,
      hash_cache,
      root_path,
      command_line_tpl,
      target_file.local_input_file_paths,
      local_target_path,
      local_depfile_path,
      print_commands,
      dir_cache,
      updm,
      target_file.local_dependency_file_paths
    );
    plan.erase(local_target_path);
  }
}

/**
 * Should be refactored as it shares plenty with `execute_update_plan`.
 */
template <typename OStream>
void output_dot_graph(
  OStream& os,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates
) {
  os << "# generated with `upd --dot-graph`" << std::endl
    << "digraph upd {" << std::endl
    << "  rankdir=\"LR\";" << std::endl;
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);

    auto const& target_file = target_descriptor.second;
    auto const& command_line_tpl = command_line_templates[target_file.command_line_ix];
    for (auto const& input_path: target_file.local_input_file_paths) {
      os << "  \"" << input_path << "\" -> \""
        << local_target_path << "\" [label=\""
        << command_line_tpl.binary_path << "\"];" << std::endl;
    }

    plan.erase(local_target_path);
  }
  os << "}" << std::endl;
}

template <typename OStream>
void output_shell_script(
  OStream& os,
  const update_map& updm,
  update_plan& plan,
  std::vector<command_line_template> command_line_templates
) {
  std::unordered_set<std::string> mked_dir_paths;
  os << "#!/bin/bash" << std::endl
    << "# generated with `upd --shell-script`" << std::endl
    << "set -ev" << std::endl << std::endl;
  while (!plan.queued_output_file_paths.empty()) {
    auto local_target_path = plan.queued_output_file_paths.front();
    plan.queued_output_file_paths.pop();
    auto target_descriptor = *updm.output_files_by_path.find(local_target_path);
    auto const& target_file = target_descriptor.second;
    auto const& command_line_tpl = command_line_templates[target_file.command_line_ix];
    auto local_dir = io::dirname_string(local_target_path);
    if (mked_dir_paths.count(local_dir) == 0) {
      shell_escape(os << "mkdir -p ", local_dir) << std::endl;
      mked_dir_paths.insert(local_dir);
    }
    auto command_line = reify_command_line(command_line_tpl, {
      .dependency_file = "/dev/null",
      .input_files = target_file.local_input_file_paths,
      .output_files = { local_target_path }
    });
    os << command_line << std::endl;
    plan.erase(local_target_path);
  }
}

struct cannot_refer_to_later_rule_error {};

struct no_source_matches_error {
  no_source_matches_error(size_t index): source_pattern_index(index) {}
  size_t source_pattern_index;
};

std::vector<std::vector<captured_string>>
crawl_source_patterns(
  const std::string& root_path,
  const std::vector<path_glob::pattern>& patterns
) {
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
    auto& fileMatches = matches[i];
    if (fileMatches.empty()) throw no_source_matches_error(i);
    std::sort(fileMatches.begin(), fileMatches.end());
  }
  return matches;
}

struct duplicate_output_error {
  std::string local_output_file_path;
  std::pair<size_t, size_t> rule_ids;
};

update_map get_update_map(
  const std::string& root_path,
  const manifest::manifest& manifest
) {
  update_map result;
  auto matches = crawl_source_patterns(root_path, manifest.source_patterns);
  std::vector<std::vector<captured_string>> rule_captured_paths(manifest.rules.size());
  std::unordered_map<std::string, size_t> rule_ids_by_output_path;
  for (size_t i = 0; i < manifest.rules.size(); ++i) {
    const auto& rule = manifest.rules[i];
    std::unordered_map<std::string, std::pair<std::vector<std::string>, std::vector<size_t>>> data_by_path;
    for (const auto& input: rule.inputs) {
      if (input.type == manifest::update_rule_input_type::rule) {
        if (input.input_ix >= i) {
          throw cannot_refer_to_later_rule_error();
        }
      }
      const auto& input_captures =
        input.type == manifest::update_rule_input_type::source
        ? matches[input.input_ix]
        : rule_captured_paths[input.input_ix];
      for (const auto& input_capture: input_captures) {
        auto local_output = substitution::resolve(rule.output.segments, input_capture);
        auto& datum = data_by_path[local_output.value];
        datum.first.push_back(input_capture.value);
        datum.second = local_output.segment_start_ids;
      }
    }
    std::unordered_set<std::string> all_dependencies;
    for (const auto& dependency: rule.dependencies) {
      if (dependency.type == manifest::update_rule_input_type::rule) {
        if (dependency.input_ix >= i) {
          throw cannot_refer_to_later_rule_error();
        }
      }
      const auto& input_captures =
        dependency.type == manifest::update_rule_input_type::source
        ? matches[dependency.input_ix]
        : rule_captured_paths[dependency.input_ix];
      for (const auto& input_capture: input_captures) {
        all_dependencies.insert(input_capture.value);
      }
    }
    auto& captured_paths = rule_captured_paths[i];
    captured_paths.resize(data_by_path.size());
    size_t k = 0;
    for (const auto& datum: data_by_path) {
      if (result.output_files_by_path.count(datum.first)) {
        throw duplicate_output_error {
          .local_output_file_path = datum.first,
          .rule_ids = { rule_ids_by_output_path.at(datum.first), i },
        };
      }
      result.output_files_by_path[datum.first] = {
        .command_line_ix = rule.command_line_ix,
        .local_input_file_paths = datum.second.first,
        .local_dependency_file_paths = all_dependencies,
      };
      rule_ids_by_output_path[datum.first] = i;
      captured_paths[k] = substitution::capture(
        rule.output.capture_groups,
        datum.first,
        datum.second.second
      );
      ++k;
    }
  }
  return result;
}

struct no_targets_error {};

manifest::manifest read_manifest(const std::string& root_path) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(root_path + io::UPDFILE_SUFFIX);
  istream_char_reader<std::ifstream> reader(file);
  json::lexer<istream_char_reader<std::ifstream>> lexer(reader);
  return manifest::parse(lexer);
}

void compile_itself(
  const std::string& root_path,
  const std::string& working_path,
  bool print_graph,
  bool update_all_files,
  const std::vector<std::string>& relative_target_paths,
  bool print_commands,
  bool print_shell_script
) {
  auto manifest = read_manifest(root_path);
  const update_map updm = get_update_map(root_path, manifest);
  const auto& output_files_by_path = updm.output_files_by_path;
  update_plan plan;

  std::vector<std::string> local_target_paths;
  for (auto const& relative_path: relative_target_paths) {
    auto local_target_path = upd::get_local_path(root_path, relative_path, working_path);
    auto target_desc = output_files_by_path.find(local_target_path);
    if (target_desc == output_files_by_path.end()) {
      throw unknown_target_error(relative_path);
    }
    build_update_plan(plan, output_files_by_path, *target_desc);
  }

  if (update_all_files) {
    for (auto const& target_desc: output_files_by_path) {
      build_update_plan(plan, output_files_by_path, target_desc);
    }
  } else if (plan.pending_output_file_paths.size() == 0) {
    throw no_targets_error();
  }
  if (print_graph) {
    output_dot_graph(std::cout, updm, plan, manifest.command_line_templates);
    return;
  }
  if (print_shell_script) {
    output_shell_script(std::cout, updm, plan, manifest.command_line_templates);
    return;
  }

  if (mkdir((root_path + "/" + CACHE_FOLDER).c_str(), 0700) != 0 && errno != EEXIST) {
    throw std::runtime_error("cannot create upd hidden directory");
  }

  std::string log_file_path = root_path + "/" + CACHE_FOLDER + "/log";
  std::string temp_log_file_path = root_path + "/" + CACHE_FOLDER + "/log_rewritten";
  update_log::cache log_cache = update_log::cache::from_log_file(log_file_path);
  file_hash_cache hash_cache;
  auto local_depfile_path = CACHE_FOLDER + "/depfile";
  auto depfile_path = root_path + '/' + local_depfile_path;
  if (mkfifo(depfile_path.c_str(), 0644) != 0 && errno != EEXIST) {
    throw std::runtime_error("cannot make depfile FIFO");
  }

  directory_cache<mkdir> dir_cache(root_path);
  execute_update_plan(log_cache, hash_cache, root_path, updm, plan,
    manifest.command_line_templates, local_depfile_path, print_commands, dir_cache);

  log_cache.close();
  update_log::rewrite_file(log_file_path, temp_log_file_path, log_cache.records());
}

template <typename OStream>
struct err_functor {
  err_functor(OStream& os, bool color_diagnostics):
    os_(os), cd_(color_diagnostics) {}
  OStream& operator()() {
    return cli::fatal_error(os_, cd_);
  }

private:
  OStream& os_;
  bool cd_;
};

int run_with_options(const cli::options& cli_opts) {
  bool color_diags =
    cli_opts.color_diagnostics == cli::color_mode::always ||
    (cli_opts.color_diagnostics == cli::color_mode::auto_ && isatty(2));
  err_functor<std::ostream> err(std::cerr, color_diags);
  try {
    if (!(
      cli_opts.action == cli::action::update ||
      cli_opts.action == cli::action::dot_graph ||
      cli_opts.action == cli::action::shell_script
    )) {
      if (!cli_opts.relative_target_paths.empty()) {
        err() << "this operation doesn't accept target arguments" << std::endl;
        return 2;
      }
      if (cli_opts.update_all_files) {
        err() << "this operation doesn't accept `--all`" << std::endl;
        return 2;
      }
    }
    if (cli_opts.update_all_files && !cli_opts.relative_target_paths.empty()) {
      err() << "cannot have both explicit targets and `--all`" << std::endl;
      return 2;
    }
    if (cli_opts.action == cli::action::version) {
      std::cout << package::NAME << " version "
                << package::VERSION << std::endl;
      return 0;
    }
    if (cli_opts.action == cli::action::help) {
      cli::print_help();
      return 0;
    }
    auto working_path = io::getcwd_string();
    auto root_path = io::find_root_path(working_path);
    if (cli_opts.action == cli::action::root) {
      std::cout << root_path << std::endl;
      return 0;
    }
    compile_itself(
      root_path,
      working_path,
      cli_opts.action == cli::action::dot_graph,
      cli_opts.update_all_files,
      cli_opts.relative_target_paths,
      cli_opts.print_commands,
      cli_opts.action == cli::action::shell_script
    );
    return 0;
  } catch (io::cannot_find_updfile_error) {
    err() << "cannot find updfile.json in the current directory or "
          << "in any of the parent directories" << std::endl;
  } catch (io::ifstream_failed_error error) {
    err() << "failed to read file `" << error.file_path << "`" << std::endl;
  } catch (update_log::corruption_error) {
    err() << "update log is corrupted; delete or revert the `.upd/log` file"
          << std::endl;
  } catch (unknown_target_error error) {
    err() << "unknown output file: " << error.relative_path << std::endl;
  } catch (relative_path_out_of_root_error error) {
    err() << "encountered a path out of the project root: "
          << error.relative_path << std::endl;
  } catch (no_targets_error error) {
    err() << "specify at least one target to update" << std::endl;
  } catch (no_source_matches_error error) {
    err() << "the source pattern #" << error.source_pattern_index
      << " matched no files on the filesystem" << std::endl;
  } catch (duplicate_output_error error) {
    err() << "the output file `" << error.local_output_file_path
      << "' is generated by the two conflicting rules #" << error.rule_ids.first
      << " and #" << error.rule_ids.second << std::endl;
  } catch (undeclared_rule_dependency_error error) {
    err() << "the output file `" << error.local_target_path
          << "' was detected to depend on the generated file `"
          << error.local_dependency_path << "'; it must be specified "
          << "explicitly in the \"dependencies\" section of the rule"
          << std::endl;
  }
  return 2;
}

int run(int argc, char *argv[]) {
  try {
    auto cli_opts = cli::parse_options(argv);
    return run_with_options(cli_opts);
  } catch (cli::unexpected_argument_error error) {
    std::cerr << "upd: fatal: invalid argument: `" << error.arg << "`" << std::endl;
    return 1;
  } catch (cli::incompatible_options_error error) {
    std::cerr << "upd: fatal: options `" << error.first_option
      << "` and `" << error.last_option << "` are in conflict" << std::endl;
    return 1;
  } catch (cli::invalid_color_mode_error error) {
    std::cerr << "upd: fatal: `" << error.value
      << "` is not a valid color mode" << std::endl;
    return 1;
  } catch (cli::option_requires_argument_error error) {
    std::cerr << "upd: fatal: option `" << error.option
      << "` requires an argument" << std::endl;
    return 1;
  }
}

}

int main(int argc, char *argv[]) {
  return upd::run(argc, argv);
}
