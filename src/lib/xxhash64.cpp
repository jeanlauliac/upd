#include "io.h"
#include "xxhash64.h"
#include <array>
#include <fstream>

namespace upd {

XXH64_hash_t hash(const std::string& str) {
  return XXH64(str.c_str(), str.size(), 0);
}

XXH64_hash_t hash_file(unsigned long long seed, const std::string& file_path) {
  upd::xxhash64 hash(seed);
  std::ifstream ifs(file_path, std::ifstream::binary);
  std::array<char, 4096> buffer;
  do {
    ifs.read(buffer.data(), buffer.size());
    hash.update(buffer.data(), ifs.gcount());
  } while (ifs.good());
  if (ifs.bad()) {
    throw io::ifstream_failed_error(file_path);
  }
  return hash.digest();
}

unsigned long long file_hash_cache::hash(const std::string& file_path) {
  auto search = cache_.find(file_path);
  if (search != cache_.end()) {
    return search->second;
  }
  auto hash = upd::hash_file(0, file_path);
  cache_.insert({file_path, hash});
  return hash;
}

void file_hash_cache::invalidate(const std::string& file_path) {
  cache_.erase(file_path);
}

}
