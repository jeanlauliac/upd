#include "parse_concurrency.h"
#include <sstream>

namespace upd {
namespace cli {

size_t parse_concurrency(const std::string &str) {
  if (str == "auto") {
    return 0;
  }
  std::istringstream iss(str);
  size_t result;
  iss >> result;
  if (iss.fail() || !iss.eof() || result == 0) {
    throw invalid_concurrency_error(str);
  }
  return result;
}

} // namespace cli
} // namespace upd
