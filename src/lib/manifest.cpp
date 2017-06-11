#include "manifest.h"

#include "istream_char_reader.h"
#include <fstream>

namespace upd {
namespace manifest {

manifest read_file(const std::string& root_path) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(root_path + io::UPDFILE_SUFFIX);
  if (!file.is_open()) {
    throw missing_manifest_error(root_path);
  }
  istream_char_reader<std::ifstream> reader(file);
  json::lexer<istream_char_reader<std::ifstream>> lexer(reader);
  return parse(lexer);
}

}
}
