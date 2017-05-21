#include "cli.h"

namespace upd {
namespace cli {

void setup_action(
  options& result,
  std::string& current_action_arg,
  const std::string& arg,
  const action new_action
) {
  if (!current_action_arg.empty()) {
    throw incompatible_options_error(current_action_arg, arg);
  }
  current_action_arg = arg;
  result.action = new_action;
}

color_mode parse_color_mode(const std::string& str) {
  if (str == "auto") {
    return color_mode::auto_;
  }
  if (str == "never") {
    return color_mode::never;
  }
  if (str == "always") {
    return color_mode::always;
  }
  throw invalid_color_mode_error(str);
}

options parse_options(const char* const argv[]) {
  options result;
  std::string action_arg;
  bool reading_options = true;
  for (++argv; *argv != nullptr; ++argv) {
    const auto arg = std::string(*argv);
    if (reading_options && arg.size() >= 1 && arg[0] == '-') {
      if (arg.size() >= 2 && arg[1] == '-') {
        if (arg == "--root") {
          setup_action(result, action_arg, arg, action::root);
        } else if (arg == "--version") {
          setup_action(result, action_arg, arg, action::version);
        } else if (arg == "--help") {
          setup_action(result, action_arg, arg, action::help);
        } else if (arg == "--dot-graph") {
          setup_action(result, action_arg, arg, action::dot_graph);
        } else if (arg == "--shell-script") {
          setup_action(result, action_arg, arg, action::shell_script);
        } else if (arg == "--color-diagnostics") {
          ++argv;
          if (*argv == nullptr) {
            throw option_requires_argument_error("--color-diagnostics");
          }
          result.color_diagnostics = parse_color_mode(*argv);
        } else if (arg == "--all") {
          result.update_all_files = true;
        } else if (arg == "--print-commands") {
          result.print_commands = true;
        } else if (arg == "--") {
          reading_options = false;
        } else {
          throw unexpected_argument_error(arg);
        }
      } else {
        throw unexpected_argument_error(arg);
      }
    } else {
      result.relative_target_paths.push_back(arg);
    }
  }
  return result;
}

void print_help() {
  std::cout << R"HELP(usage: upd [options] [targets]

Operations
  --help                  Output usage help
  --root                  Output the root directory path
  --version               Output the semantic version numbers
  --dot-graph             Output a DOT-formatted graph of the output files
  --shell-script          Output a `bash' shell script meant to updates all the
                          specified output files

Updates
  --all                   Include all the known files in the update, or graph

General options
  --color-diagnostics {auto|always|never}
                          Use ANSI color escape codes to stderr; `auto` means
                          it'll output colors if stderr is a TTY,
                          and is the default
  --                      Make the remaining of arguments targets (no options)
)HELP";
}

}
}
