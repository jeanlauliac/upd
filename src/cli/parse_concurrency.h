#pragma once

#include <string>

namespace upd {
namespace cli {

struct invalid_concurrency_error {
  invalid_concurrency_error(const std::string &value_) : value(value_) {}
  const std::string value;
};

size_t parse_concurrency(const std::string &str);

} // namespace cli
} // namespace upd
