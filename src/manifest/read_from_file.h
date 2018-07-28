#pragma once
#include "../../gen/src/manifest/manifest.h"
#include "../command_line_template.h"
#include "../json/parser.h"
#include "../path_glob/pattern.h"
#include "../string_char_reader.h"
#include "../substitution.h"

namespace upd {
namespace manifest {

/**
 * Thrown if we are trying to read the manifest of a project root but it
 * cannot be found.
 */
struct missing_manifest_error {
  missing_manifest_error(std::string root_path_)
      : root_path(std::move(root_path_)) {}
  std::string root_path;
};

template <typename Reason> struct invalid_manifest_error {
  std::string file_path;
  Reason reason;
};

manifest read_from_file(const std::string &root_path);

} // namespace manifest
} // namespace upd
