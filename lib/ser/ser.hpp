#pragma once
#include "nlohmann/json.hpp"
#include <exception>
#include <concepts>

#include "util/hash.hpp"
#include "util/log.hpp"

using json = nlohmann::json;

namespace _detail {

/** \brief Helper struct enabling an optimization when strings are used as the Id type of a GenericStore */
template<typename Id, typename T>
struct map_type_helper {
    using MapType = std::unordered_map<Id, T>;
};
/** \brief Helper specialization enabling std::string_view's to be passed as IDs instead of string references */
template<typename T>
struct map_type_helper<std::string, T> {
    using MapType = std::unordered_map<std::string, T, StringHasher, std::equal_to<>>; 
};

}

template<typename K, typename V>
using Map = typename _detail::map_type_helper<K, V>::MapType;

/** 
 * \brief Serialization / deserialization concepts and 
 * methods
 */
namespace ser {

/** 
 * \brief Concept specifying that a type can be converted to / from a
 * JSON value. A move assignment operator is recommended for the implementing type.
 * Note that a static from_json method is preferred over a constructor to avoid unneeded copies
 * when interfacing with nlohmann-json
 */
template<typename T>
concept JsonSerializable = requires(const T v) {
    std::is_default_constructible_v<T>;
    {v.to_json()} -> std::convertible_to<json>;
    {T::from_json(std::declval<T&>(), std::declval<const json&>())};
};

/**
 * \brief Concept specifying that a type can be converted to / from a string.
 * Note that if this is implemented and JsonSerializable is not, JsonSerializable will be implemented
 * using from / to string. Also note that a move assignment operator is recommended.
 */
template<typename T>
concept StringSerializable = requires(const T v) {
    {v.to_string()} -> std::convertible_to<std::string>;
    {T::from_string(std::declval<T&>(), std::declval<std::string_view>())};
};

}

namespace nlohmann {

template<ser::JsonSerializable T>
struct adl_serializer<T> {
    static inline void to_json(json& j, const T& v) {
        j = v.to_json();
    }

    static inline void from_json(const json& j, T& v) {
        T::from_json(v, j);
    }
};

template<typename T>
requires(ser::StringSerializable<T> && !ser::JsonSerializable<T>)
struct adl_serializer<T> {
    static inline void to_json(json& j, const T& v) {
        j = v.to_string();
    }

    static inline void from_json(const json& j, T& v) {
        std::string s = j.get<std::string>();
        T::from_string(v, s);
    } 
};

}