#include <cmath>
#include <lib.hpp>
#include <stdexcept>
namespace model {

constexpr id FNV_PRIME = 1099511628211ULL;
constexpr id FNV_OFFSET = 14695981039346656037ULL;

/**
 * @brief Compute the FNV-1A hash of the given string
 */
constexpr id fnv1a(const std::string_view s) {
    id hash = FNV_OFFSET;
    for(char c : s) {
        hash = (hash * FNV_PRIME) ^ c;
    }
    return hash;
}




Component::Component(const json& val) {
    this->m_id = fnv1a(val["id"].get<std::string_view>());
    val["name"].get_to(this->m_name);
}

constexpr Length Point::distance(const Point &other) const {
    const LengthUnit units = this->x.unit();
    return Length(
        units,
        std::sqrt(
            std::pow(this->x.raw_val() - other.x.raw_to(units), 2) +
            std::pow(this->y.raw_val() - other.y.raw_to(units), 2)
        )
    );
}

}
