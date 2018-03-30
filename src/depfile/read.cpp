#include "read.h"
#include "../fd_char_reader.h"
#include "../io/file_descriptor.h"
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>

namespace upd {
namespace depfile {

/**
 * State machine that updates the depfile data for each type of token.
 */
struct parse_token_handler {
  parse_token_handler(std::unique_ptr<depfile_data> &data)
      : data_(data), state_(state_t::read_target) {}
  bool end();
  bool colon();
  bool string(const std::string &file_path);
  bool new_line();

private:
  enum class state_t { read_target, read_colon, read_dep, done };
  std::unique_ptr<depfile_data> &data_;
  state_t state_;
};

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

template <typename CharReader>
std::unique_ptr<depfile_data> parse(CharReader &char_reader) {
  std::unique_ptr<depfile_data> data;
  tokenizer<CharReader> tokens(char_reader);
  parse_token_handler handler(data);
  while (tokens.template next<parse_token_handler, bool>(handler))
    ;
  return data;
}

template std::unique_ptr<depfile_data> parse(string_char_reader &);

std::unique_ptr<depfile_data> read(const std::string &file_path) {
  int fd = io::open(file_path, O_RDONLY, 0);
  io::file_descriptor input_fd(fd);
  fd_char_reader char_reader(fd);
  return parse(char_reader);
}

} // namespace depfile
} // namespace upd
