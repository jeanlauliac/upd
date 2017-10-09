#pragma once

#include <vector>
#include <string>

namespace upd {

/**
 * A command line template contains a number of variables that are replaced when
 * updating a particular file. For example `input_files` would get replaced
 * by a vector of all the input files.
 */
enum class command_line_template_variable {
  input_files,
  output_files,
  dependency_file
};

/**
 * Describe a subsequence of a command line's arguments. It starts with a
 * sequence of literals, followed by a sequence of variables.
 */
struct command_line_template_part {
  command_line_template_part() {}
  command_line_template_part(
    std::vector<std::string> literal_args_,
    std::vector<command_line_template_variable> variable_args_
  ): literal_args(literal_args_), variable_args(variable_args_) {}

  std::vector<std::string> literal_args;
  std::vector<command_line_template_variable> variable_args;
};

inline bool operator==(const command_line_template_part& left, const command_line_template_part& right) {
  return
    left.literal_args == right.literal_args &&
    left.variable_args == right.variable_args;
}

/**
 * A command line template is composed as an alternance of literal and variable
 * args. These are arranged as subsequences ("parts") that describe a pair of
 * literals, variables. There is no way right now to have a single arg to be a
 * mix of literal and variable characters. Here's an example of command line:
 *
 *     clang++ -Wall -o a.out -L /usr/lib foo.o bar.o
 *
 * This is composed of the following subsequences (or "parts"):
 *
 *     * The "-Wall", "-o" literal arguments, followed by the
 *       output file(s) variable.
 *     * The "-L", "/usr/lib" literals, followed by the input files variable.
 *
 */
struct command_line_template {
  std::string binary_path;
  std::vector<command_line_template_part> parts;
};

inline bool operator==(const command_line_template& left, const command_line_template& right) {
  return
    left.binary_path == right.binary_path &&
    left.parts == right.parts;
}

/**
 * A pair of binary, arguments, ready to get executed.
 */
struct command_line {
  std::string binary_path;
  std::vector<std::string> args;
};

/**
 * This is useful to convert distinct command line arguments into a command line
 * string, that could be used for example inside a shell. For example, spaces
 * need to be escaped since they are already used as separator between each
 * arguments.
 */
template <typename OStream>
OStream& shell_escape(OStream& os, const std::string& value) {
  for (size_t ix = 0; ix < value.size(); ++ix) {
    char c = value[ix];
    switch (c) {
      case '\0':
        os << "\\\\0";
        continue;
      case '\\':
        os << "\\\\";
        continue;
      case '"':
        os << "\\\"";
        continue;
      case '\'':
        os << "\\'";
        continue;
      case '\n':
        os << "\\\\n";
        continue;
      case '\r':
        os << "\\\\r";
        continue;
      case ' ':
        os << "\\ ";
        continue;
    }
    os << c;
  }
  return os;
}

/**
 * Convert a command line into an equivalent command line string that can be
 * used later in a shell. This is not necessary to run a command line per se,
 * that can be executed using `exec()` and the like.
 */
template <typename OStream>
OStream& operator<<(OStream& os, command_line value) {
  shell_escape(os, value.binary_path);
  for (const auto& arg: value.args) {
    shell_escape(os << ' ', arg);
  }
  return os;
}

/**
 * Describe the contextual data necessary to replace variables in a command line
 * template.
 */
struct command_line_parameters {
  std::string dependency_file;
  std::vector<std::string> input_files;
  std::vector<std::string> output_files;
};

/**
 * Specialize a command line template for a particular set of files.
 */
command_line reify_command_line(
  const command_line_template& base,
  const command_line_parameters& parameters,
  const std::string& root_path,
  const std::string& working_path
);

}
