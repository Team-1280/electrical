#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include <string_view>
#include <type_traits>
#include <vector>
#include <concepts>

using json = nlohmann::json;

namespace model {

using id = size_t;

/**
 * @brief Concept specifying the requirements for a unit type used
 * with the @ref Quantity type 
 */
template<typename T>
concept Unit = requires {
    std::convertible_to<T, size_t>;
    {T::NUM} -> std::convertible_to<size_t>;
    {T::CONV_FACTORS} -> std::convertible_to<std::array<float, T::NUM>>;
};

/**
 * @brief Concept defining requirements for types that can be used as a
 * @ref Quantity value
 */
template<typename V>
concept QuantityVal = requires(V v) {
    {v / std::declval<float>()} -> std::convertible_to<V>;
    {v * std::declval<float>()} -> std::convertible_to<V>;
};

template<typename T>
concept SerializableToString = requires(T v) {
    std::constructible_from<const std::string_view>;
    {v.to_string()} -> std::convertible_to<std::string>;
};

template<Unit U, QuantityVal V>
struct Quantity {
public:
    /**
     * @brief Construct a new quantity value from unit and value
     */
    constexpr Quantity(U unit, V val) : m_unit{unit}, m_val{val} {}
    
    /**
     * @brief Convert this quantity to one of a different unit
     * @param unit The unit to convert to
     * @return A new measurement with the passed unit
     */
    constexpr inline Quantity<U, V> to(U unit) const {
        return Quantity(unit, this->m_val / U::CONV_FACTORS[this->m_unit] * U::CONV_FACTORS[unit]);
    }
    
    /**
     * @brief Change the unit of this quantity in place, converting the stored value
     */
    constexpr inline void conv(const U unit) {
        this->m_val = this->raw_to(unit);
        this->m_unit = unit;
    }
    
    /**
     * @brief Convert this measurement into the given units
     * @param unit The units to convert to
     * @return A raw value in the passed units
     */
    constexpr inline V raw_to(const U unit) const {
        return this->m_val / U::CONV_FACTORS[this->m_unit] * U::CONV_FACTORS[unit];
    }
    
    /** Get the underlying raw value of this quantity */
    constexpr inline V raw_val() const {
        return this->m_val;
    }
    
    /** Get the units for this quantity */
    constexpr inline U unit() const {
        return this->m_unit;
    }
    
    /** Add two quantities, returning a quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator+(const Quantity<U, V>& other) const requires requires {
        {std::declval<V>() + std::declval<V>()} -> std::convertible_to<V>;
    } {
        return Quantity(this->m_unit, this->m_val + other.raw_to(this->m_unit));
    }
    /** Subtract two quantities, returning a new quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator-(const Quantity<U, V>& other) const requires requires {
        {std::declval<V>() - std::declval<V>()} -> std::convertible_to<V>;
    } {
        return Quantity(this->m_unit, this->m_val - other.raw_to(this->m_unit));
    }
    /** Divide two quantities, returning a new quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator/(const Quantity<U, V>& other) const requires requires {
        {std::declval<V>() / std::declval<V>()} -> std::convertible_to<V>;
    } {
        return Quantity(this->m_unit, this->m_val / other.raw_to(this->m_unit));
    }
    /** Multiplt two quantities, returning a new quantity with the same units as the left-hand side operand*/
    constexpr inline Quantity<U, V> operator*(const Quantity<U, V>& other) const requires requires {
        {std::declval<V>() * std::declval<V>()} -> std::convertible_to<V>;
    } {
        return Quantity(this->m_unit, this->m_val * other.raw_to(this->m_unit));
    }
    
    /** Compare two quantities */
    constexpr inline auto operator<=>(const Quantity<U, V>& other) const requires requires {
        std::declval<V>() <=> std::declval<V>();
    } {
        return this->m_val <=> other.raw_to(this->m_unit);
    }
    constexpr inline bool operator==(const Quantity<U, V>& other) const = default;

    /**
     * @brief Deserialize a quantity from a string
     * @param str The string to deserialize a value from
     * @throws std::exception If string deserialization fails
     */
    Quantity(const std::string_view str) 
    requires SerializableToString<U> && std::convertible_to<double, V> {
        size_t num_end = 0;
        double v = 0.;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.length(), v);
        this->m_val = v;
        if(ec != std::errc()) {
            throw std::invalid_argument("Bad quantity string \"" + std::string(str) + '\"');
        }
        this->m_unit = U(std::string_view(ptr));
    }
   
    /**
     * @brief Convert this quantity to a string that can be deserialized again
     * @return The string representation of this quantity
     */
    std::string to_string() const 
    requires SerializableToString<U> && std::convertible_to<V, double> {
        return std::to_string(double(this->m_val)) + this->m_unit.to_string();
    }
private:
    U m_unit;
    V m_val;
};


/**
 * @brief An enumeration of all length units 
 */
class LengthUnit {
public:
    enum UnitVal: uint8_t {
        Millimeters = 0,
        Centimeters = 1,
        Meters = 2,
        Inches = 3,
        Feet = 4
    };

    static constexpr size_t NUM = 5;
    constexpr LengthUnit(UnitVal u) : m_u{u} {}
    constexpr LengthUnit() : m_u{UnitVal::Meters} {}
    constexpr LengthUnit(const LengthUnit& other) : m_u{other.m_u} {}
    constexpr inline operator size_t() const { return static_cast<size_t>(this->m_u); }
    
    
    /**
     * @brief Convert a unit string into a corresponding length unit value
     * @throws std::invalid_argument if the passed unit string does not match any expected 
     * units
     */
    LengthUnit(const std::string_view unit_str);
    static constexpr std::array<float, NUM> CONV_FACTORS = {
        1000.,
        100.,
        1.,
        39.37,
        3.281
    };
    
    /** Convert this unit to a string */
    std::string to_string() const noexcept;
private:
    UnitVal m_u;
};

using Length = Quantity<LengthUnit, float>;

/**
 * @brief A 2D point on the workspace plane
 */
struct Point {
public:
    /** Create a new point from x and y coordinate */
    constexpr Point(const Length x, const Length y) : x{x}, y{y} {}
        
    /** 
     * @brief Deserialize a point from a JSON value
     */
    Point(const json& jval);

    constexpr bool operator==(const Point& other) const {
        return other.x == this->x && other.y == this->y;
    }
    
    /**
     * @brief Get the distance between two points, returning a distance in the units
     * of this's x coordinate
     */
    constexpr Length distance(const Point& other) const;

    Length x;
    Length y;
};


/**
 * @brief Description of a component's footprint on the 
 * workspace
 */
class Footprint {
public:

    Footprint() = default;
    
    /**
     * @brief Create a new footprint from a list of connected points
     */
    Footprint(const std::vector<Point>& pts) : m_pts{pts} {}
    Footprint(std::vector<Point>&& pts) : m_pts{std::move(pts)} {}
    
    constexpr operator std::vector<Point>&() {
        return this->m_pts;
    }

private:
    /** A vector of points that each connect to the prior one*/
    std::vector<Point> m_pts;
};


/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
private:
    /**
     * @brief Deserialize a component from a JSON value, throwing an
     * exception if the passed JSON is invalid
     */
    Component(const json& jsonval); 

public:
    Component(Component&& other) : 
        m_id{other.m_id},
        m_name{std::move(other.m_name)},
        m_fp{std::move(other.m_fp)} {}

private:
    //! @brief Unique identifier of this component type used to reference it in the model
    id m_id;
    //! @brief User-facing name of the component type
    std::string m_name;
    //! @brief Shape of the component in the workspace 
    Footprint m_fp;
};


/**  
 * @brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
    /** A reference to a component in this graph */
    struct NodeRef {
        NodeRef() = delete;
        constexpr inline size_t idx() const { return this->m_idx; }
    private:
        NodeRef(const size_t gen, const size_t idx) : m_gen{gen}, m_idx{idx} {}

        const size_t m_gen;
        const size_t m_idx;
        friend class BoardGraph;
    };
    
    /** 
     * @brief Add a component to this graph
     * @return A reference to the added node in this graph
     */
    const NodeRef add(Component&& comp);

private:
    struct Node {
        size_t count;
        id comp_id;
    };
    
    /**
     * @brief An arena containing all nodes with their counts
     */
    std::vector<Node> m_nodes;
};

}
