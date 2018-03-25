#pragma once

#include "../../src/inspect.h"
#include <iostream>

namespace testing {

class expectation_failed_error {
public:
  expectation_failed_error(const std::string& expr_string_):
    expr_string(expr_string_) {};
  const std::string expr_string;
};

enum class equality_polarity {affirmed, negated};

struct equality_expectation_failed_error {
  // equality_expectation_failed_error(
  //   const std::string& actual_,
  //   const std::string& expectation_,
  //   const std::string& actual_expr_,
  //   const std::string& expectation_expr_,
  //   equality_polarity polarity_,
  // ): actual(actual_), expectation(expectation_), actual_expr(actual_expr_),
  //    expectation_expr(expectation_expr_), polarity(polarity_) {};
  const std::string actual;
  const std::string expectation;
  const std::string actual_expr;
  const std::string expectation_expr;
  equality_polarity polarity;
};

void expect_to_be_true(bool result, const std::string& expr_string);

template <typename TActual, typename TExpectation>
void expect_equal(const TActual& actual, const TExpectation& expectation,
                  const std::string& actual_expr,
                  const std::string& expectation_expr) {
  if (actual == expectation) return;
  throw equality_expectation_failed_error{upd::inspect(actual),
    upd::inspect(expectation), actual_expr, expectation_expr, equality_polarity::affirmed};
}

template <typename TActual, typename TExpectation>
void expect_not_equal(const TActual& actual, const TExpectation& expectation,
                  const std::string& actual_expr,
                  const std::string& expectation_expr) {
  // Easier to having just `==` implemented and ignore `!=`. If it is, and is
  // inconsistent with `==` in its results, well... What we
  // could consider is test *both* operators and have a specific error for
  // detecting such inconsistencies.
  if (!(actual == expectation)) return;
  throw equality_expectation_failed_error{upd::inspect(actual),
    upd::inspect(expectation), actual_expr, expectation_expr, equality_polarity::negated};
}

enum class test_case_result { ok, not_ok };

void write_case_baseline(test_case_result result, int index, const std::string& desc);

std::string indent_string(const std::string& target, size_t indent);

template <typename TCase>
void run_case(TCase test_case, int& index, const std::string& desc) {
  ++index;
  try {
    test_case();
  } catch (expectation_failed_error error) {
    write_case_baseline(test_case_result::not_ok, index, desc);
    std::cout << "  ---" << std::endl;
    std::cout
      << "  message: |" << std::endl
      << "    expectation failed: `" << error.expr_string << "`" << std::endl;
    std::cout << "  severity: fail" << std::endl;
    std::cout << "  ..." << std::endl;
    return;
  } catch (equality_expectation_failed_error error) {
    write_case_baseline(test_case_result::not_ok, index, desc);
    std::cout << "  ---" << std::endl;
    std::cout
      << "  message: |" << std::endl
      << "    failed: `" << error.actual_expr << "` ";
    if (error.polarity == equality_polarity::negated) {
      std::cout << "not ";
    }
    std::cout
      << "to equal `"
      << error.expectation_expr << "`" << std::endl
      << "    expected:" << std::endl
      << indent_string(error.actual, 6) << std::endl
      << "    ";
    if (error.polarity == equality_polarity::negated) {
      std::cout << "not ";
    }
    std::cout
      << "to equal:" << std::endl
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
