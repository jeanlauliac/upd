#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace upd {

struct global_inspect_options {
  unsigned int indent;
  unsigned int width;
};

struct inspect_options {
  global_inspect_options& global;
  unsigned int depth;
};

// template <typename FieldMapper>
// std::string pretty_print_struct(
//   const std::string& name,
//   const inspect_options& options,
//   FieldMapper field_mapper
// ) {
//   inspect_options inner_options =
//     {.depth = options.depth + 1, .global = options.global};
//   std::map<std::string, std::string> string_map = field_mapper(inner_options);
//   std::ostringstream stream;
//   std::string indent_spaces =
//     std::string((options.depth + 1) * options.global.indent, ' ');
//   stream << name << '(';
//   if (string_map.size() == 0) {
//     stream << ')';
//     return stream.str();
//   }
//   stream << '{';
//   for (auto iter = string_map.begin(); iter != string_map.end(); ++iter) {
//     if (iter != string_map.begin()) {
//       stream << ',';
//     }
//     stream << std::endl << indent_spaces
//       << '.' << iter->first << " = " << iter->second;
//   }
//   stream << " })";
//   return stream.str();
// }

std::string inspect(size_t value, const inspect_options& options);
std::string inspect(const std::string& value, const inspect_options& options);
std::string inspect(const char* value, const inspect_options& options);

template <typename TFirst, typename TSecond>
std::string inspect_implicit_pair(
  const std::pair<TFirst, TSecond>& pair,
  const inspect_options& options
) {
  inspect_options inner_options({.depth = options.depth + 1, .global = options.global});
  std::ostringstream os;
  os << "{ " << inspect(pair.first, inner_options)
     << ", " << inspect(pair.second, inner_options) << " }";
  return os.str();
}

struct collection_inspector {
  collection_inspector(
    const std::string& class_name,
    const inspect_options& options
  ):
    indent_spaces_(std::string((options.depth + 1) * options.global.indent, ' ')),
    inner_options_({.depth = options.depth + 1, .global = options.global}),
    begin_(true)
  {
    os_ << class_name << "({";
  }

  template <typename T>
  void push_back(const T& value) {
    if (!begin_) os_ << ',';
    os_ << std::endl << indent_spaces_ << inspect(value, inner_options_);
    begin_ = false;
  }

  template <typename TFirst, typename TSecond>
  void push_back_pair(const std::pair<TFirst, TSecond>& pair) {
    if (!begin_) os_ << ',';
    os_ << std::endl << indent_spaces_ << inspect_implicit_pair(pair, inner_options_);
    begin_ = false;
  }

  std::string result() {
    os_ << " })";
    return os_.str();
  }

private:
  std::ostringstream os_;
  std::string indent_spaces_;
  inspect_options inner_options_;
  bool begin_;
};

template <typename TFirst, typename TSecond>
std::string inspect(
  const std::pair<TFirst, TSecond>& pair,
  const inspect_options& options
) {
  std::ostringstream os;
  os << "std::pair(" << inspect_implicit_pair(pair, options) << ")";
  return os.str();
}

template <typename TKey, typename TValue>
std::string inspect(
  const std::map<TKey, TValue>& map,
  const inspect_options& options
) {
  collection_inspector insp("std::map", options);
  for (auto iter = map.begin(); iter != map.end(); ++iter) {
    insp.push_back_pair(*iter);
  }
  return insp.result();
}

template <typename TValue>
std::string inspect(
  const std::vector<TValue>& collection,
  const inspect_options& options
) {
  collection_inspector insp("std::vector", options);
  for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
    insp.push_back(*iter);
  }
  return insp.result();
}

template <typename Value, size_t Size>
std::string inspect(
  const std::array<Value, Size>& collection,
  const inspect_options& options
) {
  collection_inspector insp("std::array", options);
  for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
    insp.push_back(*iter);
  }
  return insp.result();
}

template <size_t Ix, typename... Values>
typename std::enable_if<Ix == sizeof...(Values), void>::type
inspect_tuple(collection_inspector&, const std::tuple<Values...>&) {}

template <size_t Ix, typename... Values>
typename std::enable_if<Ix < sizeof...(Values), void>::type
inspect_tuple(
  collection_inspector& insp,
  const std::tuple<Values...>& tuple
) {
  insp.push_back(std::get<Ix>(tuple));
  inspect_tuple<Ix + 1, Values...>(insp, tuple);
}

template <typename... Values>
std::string inspect(
  const std::tuple<Values...>& tuple,
  const inspect_options& options
) {
  collection_inspector insp("std::tuple", options);
  inspect_tuple<0, Values...>(insp, tuple);
  return insp.result();
}

template <typename TValue>
std::string inspect(
  const std::unordered_set<TValue>& collection,
  const inspect_options& options
) {
  collection_inspector insp("std::unordered_set", options);
  for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
    insp.push_back(*iter);
  }
  return insp.result();
}

template <typename TKey, typename TValue>
std::string inspect(
  const std::unordered_map<TKey, TValue>& collection,
  const inspect_options& options
) {
  collection_inspector insp("std::unordered_map", options);
  for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
    insp.push_back_pair(*iter);
  }
  return insp.result();
}

template <typename TAny>
std::string inspect(const TAny& any, const inspect_options& options) {
  return "<unknown-object>";
}

template <typename T>
std::string inspect(const T& value) {
  global_inspect_options global = { .indent = 2, .width = 60 };
  return inspect(value, { .global = global, .depth = 0 });
}

}
