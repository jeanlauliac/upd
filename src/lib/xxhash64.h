#pragma once

#include "xxhash.h"
#include <unordered_map>
#include <vector>

namespace upd {

/**
 * Helper for using xxHash in a streaming fashion,
 * that takes care of the state automatically.
 */
struct xxhash64 {
  xxhash64(unsigned long long seed):
    state_(XXH64_createState(), XXH64_freeState) { reset(seed); }
  void reset(unsigned long long seed) { XXH64_reset(state_.get(), seed); }
  void update(const void* input, size_t length) {
    XXH64_update(state_.get(), input, length);
  }
  XXH64_hash_t digest() { return XXH64_digest(state_.get()); }
private:
  std::unique_ptr<XXH64_state_t, XXH_errorcode(*)(XXH64_state_t*)> state_;
};

/**
 * Helper for aggregating several hashes into a single digest. This is useful
 * if you want, for example, aggregate the digest of a few different source
 * files. This is not the same as doing the digest of the concatenated source
 * files, that would be sensitive to collisions.
 */
struct xxhash64_stream {
  xxhash64_stream(unsigned long long seed): hash_(seed) {}
  xxhash64_stream& operator<<(unsigned long long value) {
    hash_.update(&value, sizeof(value));
    return *this;
  }
  XXH64_hash_t digest() { return hash_.digest(); }

private:
  xxhash64 hash_;
};

/**
 * Shorthand for hashing std::string instances.
 */
XXH64_hash_t hash(const std::string& str);

/**
 * A sensible way to hash a vector is to hash each element separately
 * and hash the hashes together. We don't want to hash everything using a
 * single stream, for example strings, because collisions could happen.
 * Ex. `{ "foo", "bar" }` and `{ "f", "oobar" }` should not
 * have the same digest.
 */
template <typename T>
XXH64_hash_t hash(const std::vector<T>& target) {
  xxhash64_stream target_hash(0);
  for (auto const& item: target) {
    target_hash << hash(item);
  }
  return target_hash.digest();
}

/**
 * Hashes an entire file, fast. Since the hash will be different for
 * small changes, this is a handy way to check if a source file changed
 * since a previous update.
 */
XXH64_hash_t hash_file(unsigned long long seed, const std::string& file_path);

/**
 * Many source files, such as C++ headers, have an impact on the compilation of
 * multiple object files at a time. So it's handy to cache the hashes for these
 * source files.
 */
struct file_hash_cache {
  unsigned long long hash(const std::string& file_path);
  /**
   * After a file was updated, or we detected changes on the filesystem, we
   * want to invalidate the digest we kept track of, as it likely changed.
   */
  void invalidate(const std::string& file_path);

private:
  std::unordered_map<std::string, unsigned long long> cache_;
};

}
