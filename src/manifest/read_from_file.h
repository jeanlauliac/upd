#pragma once
#include "../command_line_template.h"
#include "../json/parser.h"
#include "../path_glob.h"
#include "../string_char_reader.h"
#include "../substitution.h"
#include "manifest.h"

namespace upd {
namespace manifest {

typedef json::lexer<string_char_reader> string_lexer;

template <typename Lexer> manifest parse(Lexer &);
extern template manifest parse(string_lexer &);

/**
 * Thrown if we are trying to read the manifest of a project root but it
 * cannot be found.
 */
struct missing_manifest_error {
  missing_manifest_error(const std::string &root_path_)
      : root_path(root_path_) {}
  const std::string root_path;
};

template <typename Reason> struct invalid_manifest_error {
  std::string file_path;
  Reason reason;
};

manifest read_from_file(const std::string &root_path);

} // namespace manifest
} // namespace upd
