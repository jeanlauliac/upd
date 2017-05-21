#pragma once

#include <iostream>
#include <vector>

namespace upd {
namespace cli {

/**
 * The basic action that the program should do.
 */
enum class action {
  dot_graph,
  help,
  root,
  shell_script,
  update,
  version,
};

struct invalid_color_mode_error {
  invalid_color_mode_error(const std::string& value): value(value) {}
  const std::string value;
};

enum class color_mode {
  always,
  auto_,
  never,
};

struct options {
  options():
    color_diagnostics(color_mode::auto_),
    action(action::update),
    update_all_files(false),
    print_commands(false) {};
  color_mode color_diagnostics;
  action action;
  std::vector<std::string> relative_target_paths;
  bool update_all_files;
  bool print_commands;
};

struct incompatible_options_error {
  incompatible_options_error(const std::string& first_option, const std::string& last_option):
    first_option(first_option), last_option(last_option) {}
  const std::string first_option;
  const std::string last_option;
};

struct unexpected_argument_error {
  unexpected_argument_error(const std::string& arg): arg(arg) {}
  const std::string arg;
};

struct option_requires_argument_error {
  option_requires_argument_error(const std::string& option): option(option) {}
  const std::string option;
};

options parse_options(const char* const argv[]);
void print_help();

template <typename OStream>
OStream& ansi_sgr(OStream& os, int sgr_code, bool use_color) {
  if (!use_color) {
    return os;
  }
  return os << "\033[" << sgr_code << "m";
}

template <typename OStream>
OStream& fatal_error(OStream& os, bool use_color) {
  os << "upd: ";
  ansi_sgr(os, 31, use_color) << "fatal:";
  return ansi_sgr(os, 0, use_color) << ' ';
}

}
}
