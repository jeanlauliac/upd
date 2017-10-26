#include "manifest.h"

#include "cli/utils.h"
#include "io.h"
#include "istream_char_reader.h"
#include "path.h"
#include <fstream>

namespace upd {
namespace manifest {

manifest read_file(const std::string &root_path,
                   const std::string &working_path, bool use_color) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  std::string file_path = root_path + io::UPDFILE_SUFFIX;
  file.open(file_path);
  if (!file.is_open()) {
    throw missing_manifest_error(root_path);
  }
  istream_char_reader<std::ifstream> reader(file);
  json::lexer<istream_char_reader<std::ifstream>> lexer(reader);
  try {
    return parse(lexer);
  } catch (json::invalid_character_error error) {
    std::cerr << cli::ansi_sgr(1, use_color)
              << get_relative_path(working_path, file_path, root_path) << ":"
              << error.location << ":" << cli::ansi_sgr({}, use_color) << " "
              << cli::ansi_sgr(31, use_color)
              << "error:" << cli::ansi_sgr({}, use_color)
              << " invalid character `" << error.chr << "`" << std::endl;
    throw invalid_manifest_error();
  }
}

} // namespace manifest
} // namespace upd
