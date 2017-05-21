#include "command_line_template.h"

namespace upd {

void reify_command_line_arg(
  std::vector<std::string>& args,
  const command_line_template_variable variable_arg,
  const command_line_parameters& parameters
) {
  switch (variable_arg) {
    case command_line_template_variable::input_files:
      args.insert(
        args.cend(),
        parameters.input_files.cbegin(),
        parameters.input_files.cend()
      );
      break;
    case command_line_template_variable::output_files:
      args.insert(
        args.cend(),
        parameters.output_files.cbegin(),
        parameters.output_files.cend()
      );
      break;
    case command_line_template_variable::dependency_file:
      args.push_back(parameters.dependency_file);
      break;
  }
}

command_line reify_command_line(
  const command_line_template& base,
  const command_line_parameters& parameters
) {
  command_line result;
  result.binary_path = base.binary_path;
  for (auto const& part: base.parts) {
    for (auto const& literal_arg: part.literal_args) {
      result.args.push_back(literal_arg);
    }
    for (auto const& variable_arg: part.variable_args) {
      reify_command_line_arg(result.args, variable_arg, parameters);
    }
  }
  return result;
}

}
