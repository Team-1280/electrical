#include <cmath>
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

constexpr id::id(std::uint8_t const * str) {
    this->m_n = FNV_OFFSET;
    while(*str != 0) {
        this->m_n = (this->m_n * FNV_PRIME) ^ *str;
        str += 1;
    }
}

constexpr id::id(const std::string_view str) {
    this->m_n = FNV_OFFSET;
    for(std::uint8_t c : str) {
        this->m_n = (this->m_n * FNV_PRIME) ^ c;
    }
}

std::string id::to_string() const {
    return std::to_string(this->m_n);
}

Footprint::Footprint(const json& val) : m_pts{} {
    for(const json& v : val) {
        this->m_pts.push_back(Point(v));
    }
}

json Footprint::to_json() const {
    json::array_t arr{};
    for(const Point& pt : this->m_pts) {
        arr.push_back(pt.to_json());
    }
    return arr;
}

Component::Component(const json& val) {
    //this->m_id = std::string_view{val["id"].get<std::string>()};
    val["name"].get_to(this->m_name);
    this->m_fp = val["footprint"];
}

json Component::to_json() const {
    return json::object({
        {"name", this->m_name},
        {"footprint", this->m_fp.to_json()}
    });
}

Point::Point(const json& val) :
    x{std::string_view{val[0].get<std::string>()}},
    y{std::string_view(val[1].get<std::string>())}
    {

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
