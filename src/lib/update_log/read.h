#pragma once

#include "../fd_char_reader.h"
#include "../json/lexer.h"
#include "../string_char_reader.h"
#include "cache.h"
#include "file_record.h"

namespace upd {
namespace update_log {

/**
 * It means the update log has been corrupted.
 */
struct unexpected_end_of_file_error {};

template <typename Reader> cache_file_data read(Reader &);
extern template cache_file_data read(string_char_reader &);
extern template cache_file_data read(fd_char_reader &);

} // namespace update_log
} // namespace upd
