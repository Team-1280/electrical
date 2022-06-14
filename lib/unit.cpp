#include <stdexcept>
#include <unit.hpp>

constexpr const std::uint64_t FNV_PRIME = 1099511628211ULL;
constexpr const std::uint64_t FNV_OFFSET = 14695981039346656037ULL;

constexpr std::uint64_t fnv1a_lowercase(const std::string_view str) {
    std::uint64_t hash = FNV_OFFSET;
    for(const std::uint8_t c : str) {
        hash = (hash ^ std::tolower(c)) * FNV_PRIME;
    }

    return hash;
}

/** Generate a compile-time constant hash, input must be lowercase */
static consteval std::uint64_t operator""_h(const char *str, std::size_t len) {
    std::uint64_t hash = FNV_OFFSET;
    for(std::size_t i = 0; i < len; ++i) {
        hash = (hash ^ str[i]) * FNV_PRIME;
    }
    return hash;
}

void LengthUnit::from_string(LengthUnit& self, std::string_view unit_str) {
    self.m_u = LengthUnit::Meters;

    while(unit_str.starts_with(' ')) {
        unit_str.remove_prefix(1);
    }
    while(unit_str.ends_with(' ')) {
        unit_str.remove_suffix(1);
    }
    if(unit_str.empty()) {
        return;
    }
    
    std::uint64_t hash = fnv1a_lowercase(unit_str);

    switch(hash) {
        case "meter"_h:
        case "m"_h:
        case "meters"_h: {
            self.m_u = LengthUnit::Meters;
        } return;

        case "centimeter"_h:
        case "centimeters"_h:
        case "cm"_h: {
            self.m_u = LengthUnit::Centimeters;
        } return;

        case "millimeter"_h:
        case "millimeters"_h:
        case "mm"_h: {
            self.m_u = LengthUnit::Millimeters;
        } return;

        case "inch"_h:
        case "inches"_h:
        case "in"_h: {
            self.m_u = LengthUnit::Inches;
        } return;

        case "foot"_h:
        case "feet"_h:
        case "ft"_h: {
            self.m_u = LengthUnit::Feet;
        } return;

        default: throw std::runtime_error{fmt::format("Invalid length unit {}", std::string{unit_str})};
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


Length operator ""_m(long double val) {
    return Length(LengthUnit::Meters, (float)val);
}
