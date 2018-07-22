#pragma once

#include "pattern.h"

namespace upd {
namespace path_glob {

/**
 * Parses a string, for example `src/(** / *).cpp`, into its corresponding
 * pattern object. Parsing has two goals:
 *
 *   * it checks that the pattern is valid, for example, that all the capture
 *     group parentheses are properly closed, or that wildcards are correct;
 *   * it generates a representation of the pattern that is easier and more
 *     performant for the pattern matcher to work with than a raw string, for
 *     example handling special characters escaping.
 */
pattern parse(std::string pattern_string);

} // namespace path_glob
} // namespace upd
