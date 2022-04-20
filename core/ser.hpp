#pragma once
#include "nlohmann/json.hpp"
#include <concepts>

using json = nlohmann::json;

/** 
 * Serialization / deserialization concepts and 
 * methods
 */
namespace ser {

/** 
 * @brief Concept specifying that a type can be converted to / from a
 * JSON value. A move assignment operator is recommended for the implementing type
 */
template<typename T>
concept JsonSerializable = requires(const T v) {
    {v.to_json()} -> std::convertible_to<json>;
    std::constructible_from<const json&>;
};

/**
 * @brief Concept specifying that a type can be converted to / from a string.
 * Note that if this is implemented and JsonSerializable is not, JsonSerializable will be implemented
 * using from / to string. Also note that a move assignment operator is recommended
 */
template<typename T>
concept StringSerializable = requires(const T v) {
    {v.to_string()} -> std::convertible_to<std::string>;
    std::constructible_from<const std::string_view>;
};

}

namespace nlohmann {

template<ser::JsonSerializable T>
struct adl_serializer<T> {
    static inline void to_json(json& j, const T& v) {
        j = v.to_json();
    }

    static inline void from_json(const json& j, T& v) {
        v = T(j);
    }
};

template<typename T>
requires(ser::StringSerializable<T> && !ser::JsonSerializable<T>)
struct adl_serializer<T> {
    static inline void to_json(json& j, const T& v) {
        j = v.to_string();
    }

    static inline void from_json(const json& j, T& v) {
        v = T(std::string_view(j.get<std::string>()));
    } 
};

}
