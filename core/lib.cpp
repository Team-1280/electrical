#include <cmath>
#include <lib.hpp>
#include <stdexcept>

static bool equal_ignore_case(const std::string_view a, const std::string_view b) {
    return std::equal(
        a.begin(), a.end(),
        b.begin(), b.end(),
        [](char a, char b) {
            return std::tolower(a) == std::tolower(b);
        }
    );
}

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


LengthUnit::LengthUnit(const std::string_view original_unit_str) {
    if(original_unit_str.empty()) {
        throw std::invalid_argument("LengthUnit#LengthUnit(string) called with empty string as argument");
    }

    const std::string_view unit_str = original_unit_str.substr(original_unit_str.find_first_not_of(' '));
        
    static const auto metric = [&](const char * const prefix, UnitVal unit) {
        if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 'm') {
            this->m_u = unit;
        } else if(unit_str.length() >= 6 && equal_ignore_case(unit_str.substr(0, 4), prefix)) {
            if((unit_str.length() == 6 && std::tolower(unit_str.at(5)) == 'm') ||
                (unit_str.length() >= 10 && equal_ignore_case(unit_str.substr(5, 5), "meter"))
            ) {
                this->m_u = unit;
            }
        } else {
            throw std::invalid_argument("LengthUnit#LengthUnit called with invalid string \"" + std::string(unit_str) + '\"');
        }
    };

    char first = std::tolower(unit_str.at(0));
    switch(first) {
        case 'c': metric("centi", UnitVal::Centimeters); break;
        case 'm': {
            if(unit_str.length() >= 6 && equal_ignore_case(unit_str, "meter")) {
                this->m_u = UnitVal::Meters;
                break;
            }
            metric("milli", UnitVal::Millimeters); 
        } break;
        case 'i': {
            if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 'n' ||
                (unit_str.length() >= 4 && equal_ignore_case(unit_str.substr(0, 5), "inch"))
            ) {
                this->m_u = UnitVal::Inches;
            }
        } break;
        case 'f': {
            if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 't' ||
                (unit_str.length() >= 4 && equal_ignore_case(unit_str.substr(0, 5), "feet"))
            ) {
                this->m_u = UnitVal::Feet;
            }
        } break;
        default: throw std::invalid_argument("LengthUnit#LengthUnit(string) called with invalid string \"" + std::string(unit_str) + '\"');
    }
}

std::string LengthUnit::to_string() const noexcept {
    switch(this->m_u) {
        case UnitVal::Feet: return "ft";
        case UnitVal::Inches: return "in";
        case UnitVal::Millimeters: return "mm";
        case UnitVal::Centimeters: return "cm";
        case UnitVal::Meters: return "m";
        default: return "";
    }
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
