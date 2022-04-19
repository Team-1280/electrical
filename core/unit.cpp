#include <unit.hpp>

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

}
