#include "lib/cli.h"
#include "lib/command_line_template.h"
#include "lib/depfile.h"
#include "lib/directory_cache.h"
#include "lib/gen_update_map.h"
#include "lib/inspect.h"
#include "lib/io.h"
#include "lib/istream_char_reader.h"
#include "lib/json/lexer.h"
#include "lib/manifest.h"
#include "lib/output_dot_graph.h"
#include "lib/path.h"
#include "lib/path_glob.h"
#include "lib/substitution.h"
#include "lib/update.h"
#include "lib/update_log.h"
#include "lib/update_plan.h"
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
#include <stdexcept>
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

struct no_targets_error {};

/**
 * Thrown if we are trying to read the manifest of a project root but it
 * cannot be found.
 */
struct missing_manifest_error {
  missing_manifest_error(const std::string& root_path): root_path(root_path) {}
  std::string root_path;
};

manifest::manifest read_manifest(const std::string& root_path) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(root_path + io::UPDFILE_SUFFIX);
  if (!file.is_open()) {
    throw missing_manifest_error(root_path);
  }
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
  const update_map updm = gen_update_map(root_path, manifest);
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

void create_root(const std::string& dir_path) {
  std::ofstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(dir_path + io::ROOTFILE_SUFFIX);
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
    if (cli_opts.action == cli::action::init) {
      create_root(working_path);
      return 0;
    }
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
  } catch (io::cannot_find_root_error) {
    err() << "cannot find a `.updroot' file in the current directory or "
          << "in any of the parent directories" << std::endl
          << "To start a new project in the current directory, "
          << "run the `upd --init' command." << std::endl;
  } catch (missing_manifest_error error) {
    err() << "could not find manifest file `updfile.json' in root path `"
          << error.root_path << "'" << std::endl
          << "Did you forget to run the project's configuration script?"
          << std::endl;
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
