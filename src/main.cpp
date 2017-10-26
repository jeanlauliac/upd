#include "../gen/src/lib/cli/parse_options.h"
#include "lib/cli/utils.h"
#include "lib/execute_manifest.h"
#include "lib/gen_update_map.h"
#include "lib/inspect.h"
#include "lib/path.h"
#include "package.h"

namespace upd {

void create_root(const std::string &dir_path) {
  std::ofstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(dir_path + io::ROOTFILE_SUFFIX);
}

template <typename OStream> struct err_functor {
  err_functor(OStream &os, bool color_diagnostics)
      : os_(os), cd_(color_diagnostics) {}
  OStream &operator()() { return cli::fatal_error(os_, cd_); }

private:
  OStream &os_;
  bool cd_;
};

size_t get_concurrency(size_t opt_concurrency) {
  if (opt_concurrency > 0) return opt_concurrency;
  auto cr = std::thread::hardware_concurrency();
  if (cr == 0) return 1;
  return cr;
}

int run_with_options(const cli::options &cli_opts, bool auto_color_diags) {
  bool color_diags =
      cli_opts.color_diagnostics == cli::color_diagnostics::always ||
      (cli_opts.color_diagnostics == cli::color_diagnostics::auto_ &&
       auto_color_diags);
  err_functor<std::ostream> err(std::cerr, color_diags);
  try {
    if (!(cli_opts.command == cli::command::update ||
          cli_opts.command == cli::command::graph ||
          cli_opts.command == cli::command::script)) {
      if (!cli_opts.rest_args.empty()) {
        err() << "this command doesn't accept target arguments" << std::endl;
        return 2;
      }
    }
    if (cli_opts.all && !cli_opts.rest_args.empty()) {
      err() << "cannot have both explicit targets and `--all`" << std::endl;
      return 2;
    }
    if (cli_opts.command == cli::command::version) {
      std::cout << "upd version " << package::VERSION << std::endl;
      return 0;
    }
    if (cli_opts.command == cli::command::help) {
      bool use_color = cli_opts.color == cli::color::always ||
                       (cli_opts.color == cli::color::auto_ && isatty(1));
      cli::output_help(cli_opts.program_name, use_color, std::cout);
      return 0;
    }
    auto working_path = io::getcwd_string();
    if (cli_opts.command == cli::command::init) {
      create_root(working_path);
      return 0;
    }
    auto root_path = io::find_root_path(working_path);
    if (cli_opts.command == cli::command::root) {
      std::cout << root_path << std::endl;
      return 0;
    }
    execute_manifest(root_path, working_path,
                     cli_opts.command == cli::command::graph, cli_opts.all,
                     cli_opts.rest_args, cli_opts.print_commands,
                     cli_opts.command == cli::command::script,
                     get_concurrency(cli_opts.concurrency), color_diags);
    return 0;
  } catch (update_failed_error error) {
    err() << "one or more files failed to update" << std::endl;
  } catch (manifest::invalid_manifest_error error) {
    err() << "invalid manifest file; correct the error(s) above" << std::endl;
  } catch (io::cannot_find_root_error) {
    err() << "cannot find a `.updroot' file in the current directory or "
          << "in any of the parent directories" << std::endl
          << "To start a new project in the current directory, "
          << "run the `upd --init' command." << std::endl;
  } catch (manifest::missing_manifest_error error) {
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
          << "' is generated by the two conflicting rules #"
          << error.rule_ids.first << " and #" << error.rule_ids.second
          << std::endl;
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
  auto color_diags = isatty(2);
  err_functor<std::ostream> err(std::cerr, color_diags);
  try {
    auto cli_opts = cli::parse_options(argv);
    return run_with_options(cli_opts, color_diags);
  } catch (cli::missing_command_error error) {
    cli::output_help(error.program_name, color_diags, std::cerr);
  } catch (cli::invalid_command_error error) {
    err() << "`" << error.value << "` is not a valid command" << std::endl;
  } catch (cli::unexpected_argument_error error) {
    err() << "invalid argument: `" << error.arg << "`" << std::endl;
  } catch (cli::invalid_color_diagnostics_error error) {
    err() << "`" << error.value << "` is not a valid color mode" << std::endl;
  } catch (cli::invalid_color_error error) {
    err() << "`" << error.value << "` is not a valid color mode" << std::endl;
  } catch (cli::option_requires_argument_error error) {
    err() << "option `" << error.option << "` requires an argument"
          << std::endl;
  } catch (cli::invalid_concurrency_error error) {
    err() << "`" << error.value
          << "` is not a valid concurrency; specify `auto`, "
          << "or a number greater than zero" << std::endl;
  }
  return 1;
}

} // namespace upd

int main(int argc, char *argv[]) { return upd::run(argc, argv); }
