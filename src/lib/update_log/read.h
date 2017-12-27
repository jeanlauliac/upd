#pragma once

#include "../json/lexer.h"
#include "../string_char_reader.h"
#include "cache.h"
#include "file_record.h"

namespace upd {
namespace update_log {

struct unexpected_end_of_file_error {};

template <typename Reader> void read(Reader &, records_by_file &);
extern template void read(string_char_reader &, records_by_file &);

} // namespace update_log
} // namespace upd
