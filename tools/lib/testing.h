#pragma once

#include "../../src/lib/inspect.h"
#include <iostream>

namespace testing {

class expectation_failed_error {
public:
  expectation_failed_error(const std::string& expr_string_):
    expr_string(expr_string_) {};
  const std::string expr_string;
};

struct equality_expectation_failed_error {
  equality_expectation_failed_error(
    const std::string& target,
    const std::string& expectation,
    const std::string& expr_string
  ): target(target), expectation(expectation), expr_string(expr_string) {};
  const std::string target;
  const std::string expectation;
  const std::string expr_string;
};

void expect(bool result, const std::string& expr_string);

template <typename TTarget, typename TExpectation>
void expect_equal(const TTarget& target, const TExpectation& expectation, const std::string& expr_string) {
  if (target == expectation) return;
  throw equality_expectation_failed_error(upd::inspect(target), upd::inspect(expectation), expr_string);
}

enum class test_case_result { ok, not_ok };

void write_case_baseline(test_case_result result, int index, const std::string& desc);

std::string indent_string(const std::string& target, size_t indent);

template <typename TCase>
void run_case(TCase test_case, int& index, const std::string& desc) {
  ++index;
  try {
    test_case();
  } catch (expectation_failed_error ex) {
    write_case_baseline(test_case_result::not_ok, index, desc);
    std::cout << "  ---" << std::endl;
    std::cout
      << "  message: |" << std::endl
      << "    expectation failed: `" << ex.expr_string << "`" << std::endl;
    std::cout << "  severity: fail" << std::endl;
    std::cout << "  ..." << std::endl;
    return;
  } catch (equality_expectation_failed_error error) {
    write_case_baseline(test_case_result::not_ok, index, desc);
    std::cout << "  ---" << std::endl;
    std::cout
      << "  message: |" << std::endl
      << "    failed: `" << error.expr_string << "`" << std::endl
      << "    expected:" << std::endl
      << indent_string(error.target, 6) << std::endl
      << "    to equal:" << std::endl
      << indent_string(error.expectation, 6) << std::endl;
    std::cout << "  severity: fail" << std::endl;
    std::cout << "  ..." << std::endl;
    return;
  }
  write_case_baseline(test_case_result::ok, index, desc);
}

void write_header();
void write_plan(int last_index);

}
