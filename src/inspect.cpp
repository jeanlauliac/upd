#include "inspect.h"

namespace upd {

std::string inspect(size_t value, const inspect_options &) {
  return std::to_string(value);
}

std::string inspect(unsigned long long value, const inspect_options &) {
  return std::to_string(value) + "ull";
}

std::string inspect(unsigned short value, const inspect_options &) {
  return std::to_string(value) + "u";
}

std::string inspect(float value, const inspect_options &) {
  return std::to_string(value) + "f";
}

std::string inspect(bool value, const inspect_options &) {
  return value ? "true" : "false";
}

std::string inspect(char value, const inspect_options &) {
  return std::to_string(value);
}

/**
 * We'll probably want to add all the proper chars that need escaping.
 */
std::string inspect(const char *value, const inspect_options &) {
  std::ostringstream stream;
  stream << '"';
  for (; *value != 0; ++value) {
    switch (*value) {
    case '\0':
      stream << "\\0";
      continue;
    case '\\':
      stream << "\\\\";
      continue;
    case '"':
      stream << "\\\"";
      continue;
    case '\n':
      stream << "\\n";
      continue;
    case '\r':
      stream << "\\r";
      continue;
    }
    stream << *value;
  }
  stream << '"';
  return stream.str();
}

std::string inspect(const std::string &value, const inspect_options &) {
  return inspect(value.c_str());
}

} // namespace upd
