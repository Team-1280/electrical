#pragma once
#include "nlohmann/json.hpp"
#include <lib.hpp>
#include <concepts>

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
    {T::from_json(std::declval<const json&>())} -> std::convertible_to<T>;
};

}

namespace nlohmann {

template<ser::JsonSerializable T>
struct adl_serializer<T> {
    static inline void to_json(json& j, const T& v) {
        j = v.to_json();
    }

    static inline void from_json(const json& j, T& v) {
        v = T::from_json(j);
    }
};

}
