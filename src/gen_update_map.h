#pragma once

#include "../gen/src/manifest/manifest.h"
#include "update.h"
#include <string>

namespace upd {

struct cannot_refer_to_later_rule_error {};

struct no_source_matches_error {
  no_source_matches_error(size_t index) : source_pattern_index(index) {}
  size_t source_pattern_index;
};

struct duplicate_output_error {
  std::string local_output_file_path;
  std::pair<size_t, size_t> rule_ids;
};

update_map gen_update_map(const std::string &root_path,
                          const manifest::manifest &manifest);

} // namespace upd
