#include "testing.h"

namespace testing {

void expect(bool result, const std::string& expr_string) {
  if (!result) {
    throw expectation_failed_error(expr_string);
  }
}

void write_case_baseline(test_case_result result, int index, const std::string& desc) {
  const char* result_str = result == test_case_result::ok ? "ok" : "not ok";
  std::cout << result_str << ' ' << index << " - " << desc << std::endl;
}

void write_header() {
  std::cout << "TAP version 13" << std::endl;
}

void write_plan(int last_index) {
  std::cout << "1.." << last_index << std::endl;
}

std::string indent_string(const std::string& target, size_t indent) {
  std::ostringstream os;
  std::string indent_spaces(indent, ' ');
  size_t prevPos = 0;
  size_t pos = target.find_first_of('\n');
  while (pos != std::string::npos) {
    os << indent_spaces << target.substr(prevPos, pos - prevPos + 1);
    prevPos = pos + 1;
    pos = target.find_first_of('\n', prevPos);
  }
  os << indent_spaces << target.substr(prevPos);
  return os.str();
}

}
