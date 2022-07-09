#include <stdexcept>
#include <unit.hpp>
#include <doctest.h>

void LengthUnit::from_string(LengthUnit& self, std::string_view unit_str) {
    self.m_u = DEFAULT;
    unit_str = _detail::trim(unit_str);
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
    unit_str = _detail::trim(unit_str);
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

TEST_CASE("Units") {
    SUBCASE("Length") {
        SUBCASE("StringSerializable") {
            Length fromstr{};
            Length::from_string(fromstr, "5.3in");
            CHECK(fromstr == Length{LengthUnit::Inches, 5.3});
            fromstr = Length{};
            Length::from_string(fromstr, "13.213");
            CHECK(fromstr == Length{LengthUnit::Meters, 13.213});
        }
    }

    SUBCASE("Mass") {
        SUBCASE("StringSerializable") {
            Mass fromstr{};
            Mass::from_string(fromstr, " 12.41 lbs");
            CHECK(fromstr == Mass{MassUnit::Pounds, 12.41});
            fromstr = Mass{};
            Mass::from_string(fromstr, " 51g");
            CHECK(fromstr == Mass{MassUnit::Milligrams, 51000});
        }
    }
}
