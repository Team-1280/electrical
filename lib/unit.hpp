#pragma once
#include <concepts>
#include <string_view>
#include <charconv>
#include <stdexcept>
#include <array>
#include <string>
#include <type_traits>

#include "ser/ser.hpp"
#include "util/log.hpp"


/**
 * \brief Concept specifying the requirements for a unit type used
 * with the Quantity type 
 */
template<typename T>
concept Unit = requires {
    {T::DEFAULT} -> std::convertible_to<T>;
    requires std::convertible_to<T, size_t>;
    {T::NUM} -> std::convertible_to<size_t>;
    {T::CONV_FACTORS} -> std::convertible_to<std::array<float, T::NUM>>;
};

/**
 * \brief Concept defining requirements for types that can be used as a
 * Quantity value
 */
template<typename V>
concept QuantityVal = requires(V v) {
    {v / std::declval<float>()} -> std::convertible_to<V>;
    {v * std::declval<float>()} -> std::convertible_to<V>;
};

/**
 * \brief A generic quantity type with unit and value type
 */
template<Unit U, QuantityVal V>
struct Quantity {
public:
    /**
     * \brief Construct a new quantity value from unit and value
     * \param unit The unit that val is in
     * \param val A measurement in the given units
     */
    explicit constexpr Quantity(U unit, V val) : m_unit{unit}, m_val{normalize(val, unit)} {}
    explicit constexpr Quantity() requires(std::default_initializable<V>) : m_unit(U::DEFAULT), m_val{} {}
    
    /**
     * \brief Create a new Quantity using the default unit
     */
    explicit inline Quantity(const V& v) : m_unit{U::DEFAULT}, m_val{v} {}
    
    /** \brief Copy construct this quantity from another quantity */
    constexpr Quantity(const Quantity& other) requires requires {
        requires std::copy_constructible<U>;
        requires std::copy_constructible<V>;
    } : m_unit{other.m_unit}, m_val{other.m_val} {}
    
    /** \brief Move construct this quantity */
    constexpr Quantity(Quantity&& other) requires(std::is_move_assignable_v<U> && std::is_move_assignable_v<V>) 
        : m_unit{std::move(other.m_unit)}, m_val{std::move(other.m_val)} {}

    /**
     * \brief Copy-construct this quantity from another quantity 
     */
    constexpr Quantity& operator=(const Quantity& other) requires requires {
        std::is_copy_assignable_v<U>;
        std::is_copy_assignable_v<V>;
    }{
        this->m_val = other.m_val;
        this->m_unit = other.m_unit;
        return *this;
    }
    
    /**
     * \brief Assign the rvalue reference to another quantity to this quantity
     */
    constexpr Quantity& operator=(Quantity&& other) requires requires {
        std::is_move_assignable_v<U>;
        std::is_move_assignable_v<V>;
    } {
        this->m_unit = std::move(other.m_unit);
        this->m_val = std::move(other.m_val);
        return *this;
    }

    /**
     * \brief Convert this quantity to one of a different unit
     * \param unit The unit to convert to
     * \return A new measurement with the passed unit
     */
    constexpr inline Quantity<U, V> to(U unit) const {
        return Quantity(unit, this->m_val);
    }
    
    /** \brief Get the length value in this quantity's units */
    constexpr inline V value() const {
        return this->m_val * U::CONV_FACTORS[this->m_unit];
    }
    
    /** 
     * \brief Get the value of this quantity in the default unit
     * \return this quantity normalized to the default unit
     */
    constexpr inline V const& normalized() const {
        return this->m_val;
    }

    /** 
     * \brief Get the value of this quantity in the default unit
     * \return this quantity normalized to the default unit
     */
    constexpr inline V& normalized() {
        return this->m_val;
    }
    
    /**
     * \brief Change the unit of this quantity in place, converting the stored value in place
     * \param unit The unit to convert to
     */
    constexpr inline void conv(const U unit) {
        this->m_unit = unit;
    }
    
    /** \brief Get the units for this quantity */
    constexpr inline U unit() const {
        return this->m_unit;
    }
    
    /** \brief Add two quantities, returning a quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator+(const Quantity<U, V>& other) const requires requires(V v) {
        {v + v} -> std::convertible_to<V>;
    } {
        return raw(this->m_unit, this->m_val + other.m_val);
    }
    /** \brief Subtract two quantities, returning a new quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator-(const Quantity<U, V>& other) const requires requires(V v) {
        {v - v} -> std::convertible_to<V>;
    } {
        return raw(this->m_unit, this->m_val - other.m_val);
    }
    /** \brief Divide two quantities, returning a new quantity with the same units as the left-hand side operand */
    constexpr inline Quantity<U, V> operator/(const Quantity<U, V>& other) const requires requires(V v) {
        {v / v} -> std::convertible_to<V>;
    } {
        return raw(this->m_unit, this->m_val / other.m_val);
    }
    /** \brief Multiply two quantities, returning a new quantity with the same units as the left-hand side operand*/
    constexpr inline Quantity<U, V> operator*(const Quantity<U, V>& other) const requires requires(V v) {
        {v * v} -> std::convertible_to<V>;
    } {
        return raw(this->m_unit, this->m_val * other.m_val);
    }

    constexpr inline Quantity<U, V>& operator+=(const Quantity<U, V>& other) requires requires(V v) { 
        {v += v}->std::convertible_to<V&>; 
    } {
        this->m_val += other.m_val;
        return *this;
    }
    constexpr inline Quantity<U, V>& operator-=(const Quantity<U, V>& other) requires requires(V v) { 
        {v -= v}->std::convertible_to<V&>; 
    } {
        this->m_val -= other.m_val;
        return *this;
    }
    constexpr inline Quantity<U, V>& operator*=(const Quantity<U, V>& other) requires requires(V v) { 
        {v *= v}->std::convertible_to<V&>; 
    } {
        this->m_val *= other.m_val;
        return *this;
    }
    constexpr inline Quantity<U, V>& operator/=(const Quantity<U, V>& other) requires requires(V v) { 
        {v /= v}->std::convertible_to<V&>; 
    } {
        this->m_val /= other.m_val;
        return *this;
    }
    
    /** \brief Scale this quantity by a floating-point value */
    constexpr inline Quantity<U, V> operator*(float scale) const {
        return raw(this->m_unit, this->m_val * scale);
    }

    constexpr inline Quantity<U, V>& operator*=(float scale) {
        this->m_val *= scale;
        return *this;
    }
    /** \brief Scale this quantity by a floating-point value */
    constexpr inline Quantity<U, V> operator/(float scale) const {
        return raw(this->m_unit, this->m_val / scale);
    }

    constexpr inline Quantity<U, V>& operator/=(float scale) {
        this->m_val /= scale;
        return *this;
    }


    /** \brief Compare two quantities */
    constexpr inline auto operator<=>(const Quantity<U, V>& other) const requires requires {
        std::declval<V>() <=> std::declval<V>();
    } {
        return this->m_val <=> other.m_val;
    }
    /** \brief Check if two Quanties are the equal */
    constexpr inline bool operator==(const Quantity<U, V>& other) const = default;

    /**
     * \brief Deserialize a quantity from a string
     * \param str The string to deserialize a value from
     * \throws std::exception If string deserialization fails
     */
    static void from_string(Quantity<U, V>& self, std::string_view str) 
    requires ser::StringSerializable<U> && std::convertible_to<double, V> {
        double v = 0.;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.length(), v);
        if(ec != std::errc()) {
            throw std::invalid_argument("Bad quantity string \"" + std::string(str) + '\"');
        }
        U::from_string(self.m_unit, str.substr(ptr - str.data()));
        self.m_val = normalize(v, self.m_unit);
    }
   
    /**
     * \brief Convert this quantity to a string that can be deserialized again
     * \return The string representation of this quantity
     */
    std::string to_string() const 
    requires ser::StringSerializable<U> && std::convertible_to<V, double> {
        return std::to_string(double(this->value())) + this->m_unit.to_string();
    }
    
    /**
     * \brief Normalize a value in the given unit to the default
     * \param val Measure in the given unit
     * \param unit Unit that val is in
     * \return A value normalized to the base unit of U
     */
    static constexpr V normalize(const V& val, const U& unit) {
        return val / U::CONV_FACTORS[unit];
    }

private:
    U m_unit;
    /** \brief Quantity value normalized to the base units of V */
    V m_val;

     /** \brief For internal use only, creates a new Quantity with the given normalized length and unit */
    static constexpr inline Quantity<U, V> raw(U unit, V val) {
        Quantity<U, V> q{val};
        q.m_unit = unit;
        return q;
    }
};

/**
 * \brief An enumeration of all length units 
 * \implements ser::StringSerializable
 * \implements Unit
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
    constexpr LengthUnit(const LengthUnit& other) = default;
    constexpr inline operator size_t() const { return static_cast<size_t>(this->m_u); }
    constexpr inline LengthUnit& operator=(const LengthUnit& other) {
        this->m_u = other.m_u;
        return *this;
    } 
    
    /**
     * \brief Convert a unit string into a corresponding length unit value
     * \throws std::invalid_argument if the passed unit string does not match any expected 
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
    
    /** \brief Convert this unit to a string */
    std::string to_string() const noexcept;

    static constexpr const UnitVal DEFAULT = UnitVal::Meters;

private:
    UnitVal m_u;
};


using Length = Quantity<LengthUnit, float>;

static_assert(ser::StringSerializable<Length>);


/** \brief Custom suffix operator for creating a new length in meters */
Length operator ""_m(long double);
