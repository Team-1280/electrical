#pragma once
#include <concepts>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <array>
#include <string>

namespace model {

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
    static void from_string(LengthUnit& self, const std::string_view unit_str);
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

}
