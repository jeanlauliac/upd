#include "io-utils.h"
#include "file_descriptor.h"
#include <array>
#include <fcntl.h>
#include <sstream>

namespace upd {
namespace io {

constexpr size_t BLOCK_SIZE = 1 << 12;

std::string read_entire_file(const std::string &file_path) {
  file_descriptor fd = io::open(file_path, O_RDONLY, 0);
  std::stringbuf sb;
  std::array<char, BLOCK_SIZE> buffer;
  size_t bytes_read;
  do {
    bytes_read = io::read(fd, buffer.data(), buffer.size());
    sb.sputn(buffer.data(), bytes_read);
  } while (bytes_read == BLOCK_SIZE);
  return sb.str();
}

void write_entire_file(const std::string &file_path,
                       const std::string &content) {
  file_descriptor fd = io::open(file_path, O_WRONLY | O_CREAT, S_IRWXU);
  for (size_t i = 0; i < file_path.size(); i += BLOCK_SIZE) {
    auto slice = content.substr(i, BLOCK_SIZE);
    io::write(fd, slice.c_str(), slice.size());
  }
}

} // namespace io
} // namespace upd
