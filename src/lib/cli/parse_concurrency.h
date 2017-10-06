#pragma once

#include <string>

namespace upd {
namespace new_cli {

struct invalid_concurrency_error {
  invalid_concurrency_error(const std::string& value): value(value) {}
  const std::string value;
};

size_t parse_concurrency(const std::string& str);

}
}
