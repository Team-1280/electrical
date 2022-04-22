#include "util/log.hpp"
#include <cmath>
#include <iostream>
#include <lib.hpp>
#include <stdexcept>

namespace model {

constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ULL;

/**
 * @brief Compute the FNV-1A hash of the given string
 */
inline constexpr std::uint64_t fnv1a(const std::string_view s) {
    std::uint64_t hash = FNV_OFFSET;
    for(char c : s) {
        hash = (hash * FNV_PRIME) ^ c;
    }
    return hash;
}

void Footprint::from_json(Footprint& self, const json& val) {
    for(const json& v : val) {
        self.m_pts.push_back(v.get<Point>());
    }
}

json Footprint::to_json() const {
    json::array_t arr{};
    for(const Point& pt : this->m_pts) {
        arr.push_back(pt.to_json());
    }
    return arr;
}


void Point::from_json(Point& self, const json& val) {
    std::cout << val << self.to_json() << std::endl;
    val[0].get_to<Length>(self.x);
    val[1].get_to<Length>(self.y);
}

json Point::to_json() const {
    return json::array({
        this->x.to_string(),
        this->y.to_string()
    });
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
