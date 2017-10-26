#include "glob.h"

namespace upd {
namespace glob {

struct matcher {
  matcher(const pattern &target, const std::string &candidate)
      : target(target), candidate(candidate), indices(nullptr) {}

  matcher(const pattern &target, const std::string &candidate,
          std::vector<size_t> &indices_)
      : target(target), candidate(candidate), indices(&indices_) {
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
   * first `bar` as being matched by the wildcard instead of the literal.
   */
  bool operator()() {
    clear();
    if (!start_new_segment()) return candidate.size() == 0;
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
   * Try to match all of the known segments within the candidate. The outer loop
   * iterate the segments, as long as we could match the current segment. The
   * inner loop tries to match both the prefix and the literal of the current
   * segment. A prefix is something like a wildcard, a literal is a plain
   * string.
   *
   * Ex. `*foo` is a segment. If we cannot match, we try again from the last
   * wildcard (this might change which is the current segment). For example when
   * trying to match `*foo` with `foxfoo`, we'll match `fo` at first, but
   * realise the `x` doesn't match. In that case, we restore the state back to
   * the wildcard plus one character, `o`. It doesn't match, so we continue,
   * until `fox` is found to be part of the wildcard, and `foo` matches the
   * literal.
   */
  bool match_all_segments() {
    bool does_match;
    do {
      do {
        does_match =
            match_prefix() && match_literal(target[segment_ix].literal);
      } while (!does_match && restore_wildcard());
      ++segment_ix;
    } while (does_match && start_new_segment());
    return does_match;
  }

  /**
   * A wildcard is always matched because it can match zero characters.
   */
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

  /**
   * A wildcard is an opportunity to match any amount of random characters we
   * want! So we keep track of them when we get one, so that later we can come
   * back to it. See the main algorithm.
   */
  void start_wildcard() {
    bookmark_ix = candidate_ix;
    last_wildcard_segment_ix = segment_ix;
    has_bookmark = true;
  }

  /**
   * A single wildcard cannot match periods, same as in a shell.
   */
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
  bool match_literal(const std::string &literal) {
    size_t literal_ix = 0;
    while (candidate_ix < candidate.size() && literal_ix < literal.size() &&
           candidate[candidate_ix] == literal[literal_ix]) {
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

  /**
   * The whole description of what we're trying to match.
   */
  const pattern &target;

  /**
   * The string that we're trying to fit into the target pattern.
   */
  const std::string &candidate;

  /**
   * For each segment of the pattern we matched, we store the index of the
   * first candidate's character that matches that segment. If it's `nullptr`
   * we don't bother with it.
   */
  std::vector<size_t> *indices;

  /**
   * The current segment we're trying to match.
   */
  size_t segment_ix;

  /**
   * The current character of the candidate we're trying to match against the
   * current segment.
   */
  size_t candidate_ix;

  /**
   * This is `true` only if we already encountered a wildcard somewhere in the
   * previous or the current segment.
   */
  bool has_bookmark;

  /**
   * The index of the candidate's character that's located right after the last
   * wildcard we matched. For example, a candidate `foobar` that we try to
   * match against `*bar`, the bookmark will start at zero, then will
   * progressively move to 3, then we'll be able to match the remaining `bar`.
   */
  size_t bookmark_ix;

  /**
   * The segment that started the last wildcard.
   */
  size_t last_wildcard_segment_ix;
};

bool match(const pattern &target, const std::string &candidate) {
  return matcher(target, candidate)();
}

bool match(const pattern &target, const std::string &candidate,
           std::vector<size_t> &indices) {
  return matcher(target, candidate, indices)();
}

} // namespace glob
} // namespace upd
