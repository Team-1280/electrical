#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <initializer_list>
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
        Millimeters = 0,
        Centimeters = 1,
        Meters = 2,
        Inches = 3,
        Feet = 4
    };
    
    LengthUnit() = delete;
    constexpr LengthUnit(Unit u) : m_u{u} {}
    constexpr LengthUnit(const LengthUnit& other) : m_u{other.m_u} {}
    constexpr LengthUnit(LengthUnit&& u) : m_u{u.m_u} {}
    
    constexpr operator Unit() const { return this->m_u; }
    /**
     * @brief Convert a length in these units to a length in meters
     * @param val A length measurement in our units
     * @return A measurement in meters
     */
    constexpr inline double to_meters(const double val) const {
        return val / FACTORS[this->m_u]; 
    }
    
    /**
     * @brief Convert a measurement from meters to this unit
     * @param m A measurement in meters
     * @return A measurement in this units
     */
    constexpr inline double from_meters(const double m) const {
        return m * FACTORS[this->m_u]; 
    }
    
    /**
     * @brief Convert a measure in this unit into different unit
     * @param unit The units to convert to
     * @param measure The measure to convert from our unit to `unit`
     */
    constexpr inline double conv(const double measure, const LengthUnit unit) const {
        return measure / FACTORS[this->m_u] * FACTORS[unit];
    }
    
    /**
     * @brief Convert a unit string into a corresponding length unit value
     * @throws std::invalid_argument if the passed unit string does not match any expected 
     * units
     */
    LengthUnit(const std::string_view unit_str);
private:
    static constexpr double FACTORS[5] = {
        1000.,
        100.,
        1.,
        39.37,
        3.281
    };
    Unit m_u;
};

/**
 * @brief A single unit on the coordinate plane, measured in meters internally 
 */
struct Length {
public:
    /**
     * @brief Create a new length of a unit 
     * @param unit The units (persistent)
     * @param measure Measure of length in the given unit
     */
    constexpr Length(const LengthUnit unit, const double measure) : m_unit{unit}, m_len{measure} {}
    constexpr Length(const Length& other) : m_len{other.m_len}, m_unit{other.m_unit} {}
    
    /**
     * @brief Create a new length from a length of a different unit
     */
    constexpr Length(const Length& other, const LengthUnit unit) : 
        m_unit{unit}, 
        m_len{other.m_unit.conv(other.m_len, unit)} {}
private:
    /** 
     * Saved unit to ensure user-entered lengths are kept in their
     * original units instead of being converted to meters
     */
    const LengthUnit m_unit;
    /** Measurement in the stored unit */
    double m_len;
};

/**
 * @brief A 2D point on the workspace plane
 */
struct Point {
public:
    
    constexpr Point(const Length (&list)[2]) : x{list[0]}, y{list[1]} {}

    Length x;
    Length y;
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
