#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <string_view>

using json = nlohmann::json;

namespace model {

using id = size_t;

/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    /**
     * @brief Deserialize a component from a JSON value, throwing an
     * exception if the passed JSON is invalid
     */
    Component(const json& jsonval); 
private:
    //! @brief Unique identifier of this component type used to reference it in the model
    id m_id;
    //! @brief User-facing name of the component type
    std::string m_name;
    //! @brief A list of links to purchase the 
    
    //! @brief Static counter used to assign IDs to created Components
    static id s_id_count;
};

/**
 * @brief An enumeration of all length units 
 */
class LengthUnit {
public:
    enum Unit: uint8_t {
        Millimeters,
        Centimeters,
        Meters,
        Inches,
        Feet
    };
    
    LengthUnit() = delete;
    constexpr LengthUnit(Unit u) : m_u{u} {}
    constexpr LengthUnit(LengthUnit&& u) : m_u{u.m_u} {}
    
    constexpr operator Unit() const { return this->m_u; }
    /**
     * @brief Convert a length in these units to a length in meters
     */
    constexpr inline double to_meters(double val) const {
        switch(this->m_u) {
            case Unit::Millimeters: return val * 1000.;
            case Unit::Centimeters: return val * 100.;
            case Unit::Meters: return val;
            case Unit::Inches: return val / 39.37;
            case Unit::Feet: return val / 3.281;
        }
    }
    
    /**
     * @brief Convert a unit string into a corresponding length unit value
     * @throws std::invalid_argument if the passed unit string does not match any expected 
     * units
     */
    LengthUnit(const std::string_view unit_str);
private:
    Unit m_u;
};

/**
 * @brief A single unit on the coordinate plane, measured in meters internally 
 */
struct Length {
public:
    Length(const LengthUnit unit, const double measure) {
        
    }

    Length(Length&& other) : m_meters{other.m_meters} {} 
private:
    double m_meters;
};


/**
 * @brief Description of a component's footprint on the 
 * workspace
 */
class Footprint {
public:
    
private:

};

}
