#pragma once
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace upd {
namespace depfile {

/**
 * Transform a stream of characters into tokens. We have to look one character
 * ahead to know when a token ends. For example a string is ended when we know
 * the next character is space.
 */
template <typename CharReader>
struct tokenizer {
  tokenizer(CharReader& char_reader): char_reader_(char_reader) { read_(); }

  template <typename handler_t, typename retval_t>
  retval_t next(handler_t& handler) {
    while (good_ && (c_ == ' ')) read_();
    if (!good_) { return handler.end(); };
    if (c_ == ':') { read_(); return handler.colon(); }
    if (c_ == '\n') { read_(); return handler.new_line(); }
    std::ostringstream oss;
    // TODO: refactor as a `do...while`, condition is always true for first pass
    while (good_ && !(c_ == ' ' || c_ == ':' || c_ == '\n')) {
      if (c_ == '\\') { read_(); }
      oss.put(c_);
      read_();
    }
    auto str = oss.str();
    if (str.size() == 0) {
      throw std::runtime_error("string token of size zero, parser is broken");
    }
    return handler.string(str);
  }

private:
  /**
   * In depfiles, the backslashes escapes anything, including syntax characters.
   * To escape, for example, spaces in a file path, 2 backslashes are required.
   * Consider a file "path with spaces/foo.cpp":
   *
   *     foo.o: path\\ with\\ spaces/foo.cpp
   *
   * This escaping behavior technically allows having newlines in file names,
   * with "\\<LF>". Whereas "\<LF>" just converts the significant newline into
   * whitespace.
   */
  void read_() {
    read_unescaped_();
    if (!good_ || c_ != '\\') return;
    read_unescaped_();
    if (!good_) {
      throw std::runtime_error("expected character after escape sequence `\\`");
    }
    if (c_ == '\n') {
      c_ = ' ';
    }
  }
  void read_unescaped_() { good_ = char_reader_.next(c_); }
  char c_;
  bool good_;
  CharReader char_reader_;
};

/**
 * Right now, only a single pair is accepted in depfiles, and the `target_path`
 * is expected to be the file being built. Additionally, only one `target_path`
 * is accepted. Eventually we may accept several `target_path` builts at the
 * same time.
 */
struct depfile_data {
  std::string target_path;
  std::vector<std::string> dependency_paths;
};

/**
 * Means at some point reading the depfile there was some unexpected character,
 * or it uses some unsupported features (see `struct depfile_data`).
 */
struct parse_error {
  parse_error(const std::string& message): message_(message) {}
  std::string message() const { return message_; };
private:
  std::string message_;
};

/**
 * State machine that updates the depfile data for each type of token.
 */
struct parse_token_handler {
  parse_token_handler(std::unique_ptr<depfile_data>& data):
    data_(data), state_(state_t::read_target) {}
  bool end();
  bool colon();
  bool string(const std::string& file_path);
  bool new_line();

private:
  enum class state_t { read_target, read_colon, read_dep, done };
  std::unique_ptr<depfile_data>& data_;
  state_t state_;
};

/**
 * Parse a depfile stream. A 'depfile' is a Makefile-like representation of file
 * dependencies. For example it is generated by the -MF option of gcc or clang,
 * in which case it describes what source and headers a compiled object file
 * depends on. Ex:
 *
 *     foo.o: foo.cpp \
 *       some_header.h \
 *       another_header.h
 *
 * If the stream only has whitespace, this is considered valid but will return
 * an empty pointer. Technically, several lines of "target: dependencies" would
 * be valid syntax, so this function may be improved to return a vector of
 * these instead.
 */
template <typename CharReader>
std::unique_ptr<depfile_data> parse(CharReader& char_reader) {
  std::unique_ptr<depfile_data> data;
  tokenizer<CharReader> tokens(char_reader);
  parse_token_handler handler(data);
  while (tokens.template next<parse_token_handler, bool>(handler));
  return data;
}

/**
 * A helper to read depfiles from a specific path.
 */
std::unique_ptr<depfile_data> read(const std::string& depfile_path);

}
}
