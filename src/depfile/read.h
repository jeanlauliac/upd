#pragma once
#include "../string_char_reader.h"
#include "tokenizer.h"
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

namespace upd {
namespace depfile {

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
  parse_error(const std::string &message) : message_(message) {}
  std::string message() const { return message_; };

private:
  std::string message_;
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
std::unique_ptr<depfile_data> parse(CharReader &char_reader);

extern template std::unique_ptr<depfile_data> parse(upd::string_char_reader &);

/**
 * Read the specified file as a depfile.
 */
std::unique_ptr<depfile_data> read(const std::string &file_path);

} // namespace depfile
} // namespace upd