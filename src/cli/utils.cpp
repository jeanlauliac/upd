#include "utils.h"

namespace upd {
namespace cli {

std::string ansi_sgr(std::vector<size_t> sgr_codes, bool use_color) {
  if (!use_color) {
    return "";
  }
  std::ostringstream os;
  os << "\033[";
  bool first = true;
  for (auto code : sgr_codes) {
    if (!first) os << ';';
    os << code;
    first = false;
  }
  os << "m";
  return os.str();
}

std::string ansi_sgr(size_t sgr_code, bool use_color) {
  std::vector<size_t> codes(1);
  codes[0] = sgr_code;
  return ansi_sgr(codes, use_color);
}

} // namespace cli
} // namespace upd
