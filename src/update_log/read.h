#pragma once

#include "../../gen/src/update_log/file_record.h"
#include "../fd_char_reader.h"
#include "../json/lexer.h"
#include "../string_char_reader.h"
#include "cache.h"

namespace upd {
namespace update_log {

/**
 * The log was written with another version of the tool.
 */
struct version_mismatch_error {};

/**
 * It means the update log has been corrupted.
 */
struct unexpected_end_of_file_error {};

enum class record_type : char { entity_name, file_update };

template <typename Reader> cache_file_data read(Reader &);
extern template cache_file_data read(string_char_reader &);
extern template cache_file_data read(fd_char_reader &);

} // namespace update_log
} // namespace upd