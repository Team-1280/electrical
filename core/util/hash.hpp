#pragma once

#include <string>

/** 
 * @brief Custom hasher class needed because C++ unordered_maps
 * don't support heterogeneous lookup by default
 */
struct StringHasher {
  using is_transparent = void;
  std::size_t operator()(const char *txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  std::size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  std::size_t operator()(const std::string &txt) const {
    return std::hash<std::string>{}(txt);
  }
};
