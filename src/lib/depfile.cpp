#include "depfile.h"
#include "istream_char_reader.h"

namespace upd {
namespace depfile {

bool parse_token_handler::end() {
  if (!(
    state_ == state_t::read_target ||
    state_ == state_t::read_dep ||
    state_ == state_t::done
  )) {
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

bool parse_token_handler::string(const std::string& file_path) {
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

std::unique_ptr<depfile_data> read(const std::string& depfile_path) {
  std::ifstream depfile;
  depfile.exceptions(std::ifstream::badbit);
  depfile.open(depfile_path);
  istream_char_reader<std::ifstream> char_reader(depfile);
  return parse(char_reader);
}

}
}
