#pragma once

#include "../string_char_reader.h"
#include "file_record.h"
#include "../json/lexer.h"

namespace upd {
namespace update_log {

template <typename Lexer> file_record parse(Lexer &);
extern template file_record parse(json::lexer<string_char_reader> &);

}
}
