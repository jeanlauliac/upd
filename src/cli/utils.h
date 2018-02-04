#pragma once

#include <iostream>
#include <sstream>
#include <vector>

namespace upd {
namespace cli {

std::string ansi_sgr(std::vector<size_t> sgr_codes, bool use_color);
std::string ansi_sgr(size_t sgr_code, bool use_color);

template <typename OStream>
OStream &ansi_sgr(OStream &os, std::vector<size_t> sgr_codes, bool use_color) {
  return os << ansi_sgr(sgr_codes, use_color);
}

template <typename OStream> OStream &fatal_error(OStream &os, bool use_color) {
  os << "upd: ";
  ansi_sgr(os, {31}, use_color) << "fatal:";
  return ansi_sgr(os, {0}, use_color) << ' ';
}

} // namespace cli
} // namespace upd
