#include "glob.h"

namespace upd {
namespace glob {

struct matcher {
  matcher(const pattern& target, const std::string& candidate):
    target(target),
    candidate(candidate),
    indices(nullptr) {}

  matcher(
    const pattern& target,
    const std::string& candidate,
    std::vector<size_t>& indices_
  ):
    target(target),
    candidate(candidate),
    indices(&indices_) {
      indices->resize(target.size());
    }

  /**
   * A glob pattern is matched only if we can find a sequence of segments
   * that matches, and if we reached the very end of the candidate doing so.
   * If we don't, we still have a chance to recover by restoring the state
   * to the last wildcard and trying again shifted by one character.
   *
   * Take for example, the pattern `foo*bar` and the candidate `foobarglobar`.
   * At first we'd match `foobar`, but since it doesn't match the whole
   * candidate, it's not correct. In that case we go back, and we consider the
   * first "foo" as being matched by the wildcard instead of the literal.
   */
  bool operator()() {
    clear();
    if (!start_new_segment()) return false;
    bool does_match, fully_matched;
    do {
      does_match = match_all_segments();
      fully_matched = does_match && candidate_ix == candidate.size();
    } while (does_match && !fully_matched && restore_wildcard());
    return fully_matched;
  }

  /**
   * Reset everything to the beginning as we start the algo.
   */
  void clear() {
    segment_ix = candidate_ix = bookmark_ix = last_wildcard_segment_ix = 0;
    has_bookmark = false;
  }

  /**
   * Try to match all of the known segments within the candidate. For each
   * segment we match the literal. If it doesn't match, we may be able to
   * go back and include more characters in a wildcard.
   */
  bool match_all_segments() {
    bool does_match;
    do {
      do {
        does_match = match_prefix() && match_literal(target[segment_ix].literal);
      } while (!does_match && restore_wildcard());
      ++segment_ix;
    } while (does_match && start_new_segment());
    return does_match;
  }

  bool match_prefix() {
    switch (target[segment_ix].prefix) {
      case placeholder::none:
      case placeholder::wildcard:
        return true;
      case placeholder::single_wildcard:
        return match_single_wildcard();
    }
  }

  bool start_new_segment() {
    if (segment_ix == target.size()) return false;
    if (indices) (*indices)[segment_ix] = candidate_ix;
    if (target[segment_ix].prefix == placeholder::wildcard) start_wildcard();
    return true;
  }

  void start_wildcard() {
    bookmark_ix = candidate_ix;
    last_wildcard_segment_ix = segment_ix;
    has_bookmark = true;
  }

  bool match_single_wildcard() {
    if (candidate_ix == candidate.size() || candidate[candidate_ix] == '.') {
      return false;
    }
    ++candidate_ix;
    return true;
  }

  /**
   * Just match the literal character by character.
   */
  bool match_literal(const std::string& literal) {
    size_t literal_ix = 0;
    while (
      candidate_ix < candidate.size() &&
      literal_ix < literal.size() &&
      candidate[candidate_ix] == literal[literal_ix]
    ) {
      ++candidate_ix;
      ++literal_ix;
    }
    return literal_ix == literal.size();
  }

  bool restore_wildcard() {
    if (!has_bookmark) return false;
    ++bookmark_ix;
    candidate_ix = bookmark_ix;
    segment_ix = last_wildcard_segment_ix;
    if (candidate_ix + target[segment_ix].literal.size() > candidate.size())
      return false;
    return true;
  }

  const pattern& target;
  const std::string& candidate;
  std::vector<size_t>* indices;
  size_t segment_ix;
  size_t candidate_ix;
  size_t bookmark_ix;
  size_t last_wildcard_segment_ix;
  bool has_bookmark;
};

bool match(const pattern& target, const std::string& candidate) {
  return matcher(target, candidate)();
}

bool match(
  const pattern& target,
  const std::string& candidate,
  std::vector<size_t>& indices
) {
  return matcher(target, candidate, indices)();
}

}
}
