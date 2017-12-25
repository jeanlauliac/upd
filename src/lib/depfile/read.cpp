#include "read.h"
#include "../fd_char_reader.h"
#include "../file_descriptor.h"
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>

namespace upd {
namespace depfile {

bool parse_token_handler::end() {
  if (!(state_ == state_t::read_target || state_ == state_t::read_dep ||
        state_ == state_t::done)) {
    throw parse_error("unexpected end");
  }
  state_ = state_t::done;
  return false;
}

bool parse_token_handler::colon() {
  if (state_ != state_t::read_colon) {
    throw parse_error("unexpected colon operator");
  }
  state_ = state_t::read_dep;
  return true;
}

bool parse_token_handler::string(const std::string &file_path) {
  if (state_ == state_t::read_target) {
    data_.reset(new depfile_data());
    data_->target_path = file_path;
    state_ = state_t::read_colon;
    return true;
  }
  if (state_ == state_t::read_dep) {
    data_->dependency_paths.push_back(file_path);
    return true;
  }
  throw parse_error("unexpected string `" + file_path + "`");
}

bool parse_token_handler::new_line() {
  if (state_ == state_t::read_target) {
    return true;
  }
  if (state_ != state_t::read_dep) {
    throw parse_error("unexpected newline");
  }
  state_ = state_t::done;
  return true;
}

std::unique_ptr<depfile_data> read(const std::string &file_path) {
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd < 0) {
    throw new std::runtime_error("open() failed");
  }
  file_descriptor input_fd(fd);
  fd_char_reader char_reader(fd);
  return parse(char_reader);
}

} // namespace depfile
} // namespace upd
