#include "cli.h"
#include <sstream>

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

size_t parse_concurrency(const std::string& str) {
  if (str == "auto") {
    return 0;
  }
  std::istringstream iss(str);
  size_t result;
  iss >> result;
  if (iss.fail() || !iss.eof() || result == 0) {
    throw invalid_concurrency_error(str);
  }
  return result;
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
        } else if (arg == "--init") {
          setup_action(result, action_arg, arg, action::init);
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
        } else if (arg == "--concurrency") {
          ++argv;
          if (*argv == nullptr)
            throw option_requires_argument_error("--concurrency");
          result.concurrency = parse_concurrency(*argv);
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
  std::cout << R"HELP(Usage: upd [options] [targets]
Update files according with a manifest.

Operations:
  --help          output usage help
  --root          output the root directory path
  --version       output the semantic version numbers
  --init          create a root directory at the current working path
  --dot-graph     output a DOT-formatted graph of the output files
  --shell-script  output a `bash' shell script meant to updates all
                  the specified output files

Updates:
  --all     include all the known files in the update, or graph
  --concurrency {auto|<number>}
      specify how many threads can be run in parallel, at maximum

General:
  --color-diagnostics {auto|always|never}
      use ANSI color escape codes to stderr; `auto` means it'll output colors
      if stderr is a TTY, and is the default
)HELP";
}

}
}
