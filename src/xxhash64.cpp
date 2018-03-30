#include "xxhash64.h"
#include "io/file_descriptor.h"
#include "path.h"
#include <array>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace upd {

XXH64_hash_t hash(const std::string &str) {
  return XXH64(str.c_str(), str.size(), 0);
}

constexpr size_t BLOCK_SIZE = 1 << 12;

XXH64_hash_t hash_file(XXH64_hash_t seed, const std::string &file_path) {
  upd::xxhash64 hash(seed);
  io::file_descriptor fd = io::open(file_path, O_RDONLY, 0);
  std::array<char, BLOCK_SIZE> buffer;
  size_t bytes_read;
  do {
    bytes_read = io::read(fd, buffer.data(), buffer.size());
    hash.update(buffer.data(), bytes_read);
  } while (bytes_read == BLOCK_SIZE);
  return hash.digest();
}

XXH64_hash_t file_hash_cache::hash(const std::string &file_path) {
  if (!is_path_absolute(file_path)) {
    throw std::runtime_error("expected absolute path");
  }
  auto search = cache_.find(file_path);
  if (search != cache_.end()) {
    return search->second;
  }
  auto hash = upd::hash_file(0, file_path);
  cache_.insert({file_path, hash});
  return hash;
}

void file_hash_cache::invalidate(const std::string &file_path) {
  cache_.erase(file_path);
}

} // namespace upd
