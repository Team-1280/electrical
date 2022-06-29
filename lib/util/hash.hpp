#pragma once

#include <string>

/** 
 * \brief Custom hasher class needed because C++ unordered_maps
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

constexpr const std::uint64_t FNV_PRIME = 1099511628211ULL;
constexpr const std::uint64_t FNV_OFFSET = 14695981039346656037ULL;

/**
 * \brief Compute the hash of `str`, converting each letter to lowercase before hashing
 */
constexpr std::uint64_t fnv1a_lowercase(const std::string_view str) {
    std::uint64_t hash = FNV_OFFSET;
    for(const std::uint8_t c : str) {
        hash = (hash ^ std::tolower(c)) * FNV_PRIME;
    }

    return hash;
}

/** Generate a compile-time constant hash of the given string */
static consteval std::uint64_t operator""_h(const char *str, std::size_t len) {
    std::uint64_t hash = FNV_OFFSET;
    for(std::size_t i = 0; i < len; ++i) {
        hash = (hash ^ str[i]) * FNV_PRIME;
    }
    return hash;
}
