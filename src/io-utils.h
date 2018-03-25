#pragma once
#include "io.h"
#include <string>

namespace upd {
namespace io {

std::string read_entire_file(const std::string &file_path);
void write_entire_file(const std::string &file_path,
                       const std::string &content);

} // namespace io
} // namespace upd
