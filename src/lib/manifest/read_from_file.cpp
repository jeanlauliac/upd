#include "read_from_file.h"

#include "../cli/utils.h"
#include "../io.h"
#include "../istream_char_reader.h"
#include "../path.h"
#include <fstream>

namespace upd {
namespace manifest {

manifest read_from_file(const std::string &root_path) {
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
    throw invalid_manifest_error<json::invalid_character_error>{file_path,
                                                                error};
  } catch (json::unexpected_punctuation_error error) {
    throw invalid_manifest_error<json::unexpected_punctuation_error>{file_path,
                                                                     error};
  } catch (json::unexpected_number_error error) {
    throw invalid_manifest_error<json::unexpected_number_error>{file_path,
                                                                error};
  }
}

} // namespace manifest
} // namespace upd
