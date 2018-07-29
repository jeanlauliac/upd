#include "command_line_template.h"
#include "path.h"

namespace upd {

void insert_arg_paths(std::vector<std::string> &args,
                      const std::vector<std::string> &paths,
                      const std::string &root_path,
                      const std::string &working_path) {
  for (const auto &path : paths) {
    args.push_back(get_relative_path(working_path, path, root_path));
  }
}

void reify_command_line_arg(std::vector<std::string> &args,
                            const command_line_template_variable variable_arg,
                            const command_line_parameters &parameters,
                            const std::string &root_path,
                            const std::string &working_path) {
  switch (variable_arg) {
  case command_line_template_variable::input_files:
    insert_arg_paths(args, parameters.input_files, root_path, working_path);
    break;
  case command_line_template_variable::output_files:
    insert_arg_paths(args, parameters.output_files, root_path, working_path);
    break;
  case command_line_template_variable::depfile:
    args.push_back(parameters.depfile);
    break;
  }
}

command_line reify_command_line(const command_line_template &base,
                                const command_line_parameters &parameters,
                                const std::string &root_path,
                                const std::string &working_path) {
  command_line result;
  result.binary_path = base.binary_path;
  result.environment = base.environment;
  for (auto const &part : base.parts) {
    for (auto const &literal_arg : part.literal_args) {
      result.args.push_back(literal_arg);
    }
    for (auto const &variable_arg : part.variable_args) {
      reify_command_line_arg(result.args, variable_arg, parameters, root_path,
                             working_path);
    }
  }
  return result;
}

} // namespace upd
