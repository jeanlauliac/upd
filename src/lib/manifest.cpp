#include "manifest.h"

#include "cli/utils.h"
#include "io.h"
#include "istream_char_reader.h"
#include "path.h"
#include <fstream>

namespace upd {
namespace manifest {

struct error_context {
  const std::string &working_path;
  const std::string &root_path;
  const std::string &file_path;
  const bool use_color;
};

struct error_header {
  const error_context &context;
  const json::location &location;
};

std::ostream &operator<<(std::ostream &os, const error_header &target) {
  const auto &c = target.context;
  os << cli::ansi_sgr(1, c.use_color)
     << get_relative_path(c.working_path, c.file_path, c.root_path) << ":"
     << target.location << ":" << cli::ansi_sgr({}, c.use_color) << " "
     << cli::ansi_sgr(31, c.use_color)
     << "error:" << cli::ansi_sgr({}, c.use_color);
  return os;
}

manifest read_file(const std::string &root_path,
                   const std::string &working_path, bool use_color) {
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  std::string file_path = root_path + io::UPDFILE_SUFFIX;
  error_context ectx{working_path, root_path, file_path, use_color};
  file.open(file_path);
  if (!file.is_open()) {
    throw missing_manifest_error(root_path);
  }
  istream_char_reader<std::ifstream> reader(file);

  json::lexer<istream_char_reader<std::ifstream>> lexer(reader);
  auto &es = std::cerr;
  try {
    return parse(lexer);
  } catch (json::invalid_character_error error) {
    es << error_header{ectx, error.location} << " invalid character `"
       << error.chr << "`" << std::endl;
  } catch (json::unexpected_punctuation_error error) {
    es << error_header{ectx, error.location} << " unexpected punctuation `"
       << json::to_char(error.type) << "`";
    switch (error.situation) {
    case json::unexpected_punctuation_situation::expression:
      es << "; expected to see an expression (ex. string, object, array, "
            "number)";
      break;
    case json::unexpected_punctuation_situation::field_colon:
      es << "; expected `:` followed by the field's value";
      break;
    case json::unexpected_punctuation_situation::field_name:
      es << "; expected the name of the object's next field";
      break;
    case json::unexpected_punctuation_situation::first_field_name:
      es << "; expected a field name, or an empty object";
      break;
    case json::unexpected_punctuation_situation::first_item:
      es << "; expected an array item, or an empty array";
      break;
    case json::unexpected_punctuation_situation::post_field:
      es << "; expected a comma, or the object's end";
      break;
    case json::unexpected_punctuation_situation::post_item:
      es << "; expected a comma, or the array's end";
      break;
    }
    es << std::endl;
  } catch (json::unexpected_number_error error) {
    es << error_header{ectx, error.location.from} << " unexpected number"
       << std::endl;
  }
  throw invalid_manifest_error();
}

} // namespace manifest
} // namespace upd
