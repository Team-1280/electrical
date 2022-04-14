#include <lib.hpp>

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

id Component::s_id_count = 0;

LengthUnit::LengthUnit(const std::string_view unit_str) {
    if(unit_str.empty()) {
            throw std::invalid_argument("LengthUnit#LengthUnit(string) called with empty string as argument");
        }
        
        static const auto metric = [&](const char * const prefix, Unit unit) {
            if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 'm') {
                this->m_u = unit;
            } else if(unit_str.length() >= 6 && equal_ignore_case(unit_str.substr(0, 4), prefix)) {
                if((unit_str.length() == 6 && std::tolower(unit_str.at(5)) == 'm') ||
                    (unit_str.length() >= 10 && equal_ignore_case(unit_str.substr(5, 5), "meter"))
                ) {
                    this->m_u = unit;
                }
            } else {
                throw std::invalid_argument("LengthUnit#LengthUnit called with invalid string");
            }
        };

        char first = std::tolower(unit_str.at(0));
        switch(first) {
            case 'c': metric("centi", Unit::Centimeters); break;
            case 'm': {
                if(unit_str.length() >= 6 && equal_ignore_case(unit_str, "meter")) {
                    this->m_u = Unit::Meters;
                    break;
                }
                metric("milli", Unit::Millimeters); 
            } break;
            case 'i': {
                if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 'n' ||
                    (unit_str.length() >= 4 && equal_ignore_case(unit_str.substr(0, 5), "inch"))
                ) {
                    this->m_u = Unit::Inches;
                }
            } break;
            case 'f': {
                if(unit_str.length() == 2 && std::tolower(unit_str.at(1)) == 't' ||
                    (unit_str.length() >= 4 && equal_ignore_case(unit_str.substr(0, 5), "feet"))
                ) {
                    this->m_u = Unit::Feet;
                }
            } break;
        }
            
        throw std::invalid_argument("LengthUnit#LengthUnit(string) called with invalid string");
    }

Component::Component(const json& val) {
    this->m_id = s_id_count;
    s_id_count += 1;

    val["name"].get_to(this->m_name);
}

}
