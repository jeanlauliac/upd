#pragma once

#include <iostream>

  namespace upd {
namespace cli {

template <typename OStream>
OStream& ansi_sgr(OStream& os, int sgr_code, bool use_color) {
  if (!use_color) {
    return os;
  }
  return os << "\033[" << sgr_code << "m";
}

template <typename OStream>
OStream& fatal_error(OStream& os, bool use_color) {
  os << "upd: ";
  ansi_sgr(os, 31, use_color) << "fatal:";
  return ansi_sgr(os, 0, use_color) << ' ';
}

}
}
