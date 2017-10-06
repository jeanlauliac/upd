#pragma once

#include <iostream>
#include <vector>

namespace upd {
namespace cli {

template <typename OStream>
OStream& ansi_sgr(OStream& os, std::vector<size_t> sgr_codes, bool use_color) {
  if (!use_color) {
    return os;
  }
  os << "\033[";
  bool first = true;
  for (auto code: sgr_codes) {
    if (!first) os << ';';
    os << code;
    first = false;
  }
  return os << "m";
}

template <typename OStream>
OStream& fatal_error(OStream& os, bool use_color) {
  os << "upd: ";
  ansi_sgr(os, { 31 }, use_color) << "fatal:";
  return ansi_sgr(os, { 0 }, use_color) << ' ';
}

}
}
