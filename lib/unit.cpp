#include <stdexcept>
#include <unit.hpp>

static inline std::string_view trim(const std::string_view ostr) {
    std::string_view str{ostr};
    while(str.starts_with(' ')) {
        str.remove_prefix(1);
    }
    while(str.ends_with(' ')) {
        str.remove_suffix(1);
    }
    return str;
}

void LengthUnit::from_string(LengthUnit& self, std::string_view unit_str) {
    self.m_u = DEFAULT;
    unit_str = trim(unit_str);
    if(unit_str.empty()) return;
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

void MassUnit::from_string(MassUnit &self, std::string_view unit_str) {
    self.m_u = DEFAULT;
    unit_str = trim(unit_str);
    if(unit_str.empty()) { return; }
    std::uint64_t hash = fnv1a_lowercase(unit_str);
    switch(hash) {
        case "gram"_h:
        case "grams"_h:
        case "g"_h: {
            self.m_u = UnitVal::Grams;
        } return;

        case "milligram"_h:
        case "milligrams"_h:
        case "mg"_h: {
            self.m_u = UnitVal::Milligrams;
        } return;

        case "kilogram"_h:
        case "kilograms"_h:
        case "kilos"_h:
        case "kg"_h: {
            self.m_u = UnitVal::Kilograms;
        } return;

        case "pound"_h:
        case "pounds"_h:
        case "lb"_h:
        case "lbs"_h: {
            self.m_u = UnitVal::Pounds;
        } return;

        case "ounce"_h:
        case "ounces"_h:
        case "oz"_h: {
            self.m_u = UnitVal::Ounces;
        } return;

        default: throw std::runtime_error{fmt::format("Invalid mass unit {}", std::string{unit_str})};
    }
}

std::string MassUnit::to_string() const noexcept {
    switch(this->m_u) {
        case UnitVal::Grams: return "g";
        case UnitVal::Milligrams: return "mg";
        case UnitVal::Kilograms: return "kg";
        case UnitVal::Pounds: return "lb";
        case UnitVal::Ounces: return "oz";
        default: return "";
    }
}


Length operator ""_m(long double val) {
    return Length(LengthUnit::Meters, (float)val);
}

#include <doctest.h>

TEST_CASE("Length Operations") {
    Length a{LengthUnit::Feet, 12};
    CHECK_EQ(a, Length{LengthUnit::Feet, 12});
}
