#pragma once

#include "fmt/format.h"
#include <charconv>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace _detail {

/**
 * \brief Compile-time function to raise a number to a given power,
 * only works with fixed-point numbers
 */
template<typename T>
inline consteval T pow(T num, std::size_t power) noexcept {
    return (power > 0) ? num * pow(num, power - 1) : 1;
}

}

/** 
 * \brief Class representing an amount of US dollars as a dollars and cents amount to avoid
 * floating-point imprecision
 */
class USD {
public:
    using storage = std::uint64_t;
    static constexpr const storage DEC_PLACES = 6;
    static constexpr const storage DOLLARS_SCALE = _detail::pow<storage>(10, DEC_PLACES);
    static constexpr const storage CENTS_SCALE = _detail::pow<storage>(10, DEC_PLACES - 2);

    /** \brief Create a new dollar amount from a floating-point number */
    explicit inline constexpr USD(double val) : m_dec{static_cast<storage>(std::round(val * DOLLARS_SCALE))} {}
    /** \brief Create a new dollar amount from dollars and cents */
    inline constexpr USD(storage dollars, storage cents = 0) : m_dec{dollars * DOLLARS_SCALE + cents * CENTS_SCALE} {}
    /** \brief Create a new USD amount containing $0.00 */
    inline constexpr USD() = default;

    /** \brief Get the number of whole dollars in this decimal amount */
    inline constexpr storage dollars() const noexcept { return this->m_dec / DOLLARS_SCALE; }
    /** \brief Set the dollar amount for this currency */
    inline constexpr void dollars(storage dollars) noexcept {
        this->m_dec %= DOLLARS_SCALE;
        this->m_dec += dollars * DOLLARS_SCALE;
    }
    /** \brief Get the number of cents in whole dollars of this decimal amount */
    inline constexpr storage cents() const noexcept { return this->m_dec % DOLLARS_SCALE / CENTS_SCALE; }
    /** \brief Set the amount of cents in this currency */
    inline constexpr void cents(storage cents) noexcept {
        //Looks like it does nothing, but this clears the cents digits by truncating them with division
        this->m_dec = this->m_dec / DOLLARS_SCALE * DOLLARS_SCALE;
        this->m_dec += cents * CENTS_SCALE;
    }

    /** \brief Convert this USD amount to a double value */
    explicit inline constexpr operator double() const noexcept { return static_cast<double>(this->m_dec) / this->DOLLARS_SCALE; }
    
    /** \brief Convert this USD amount to a string */
    std::string to_string() const {
        return fmt::format(
            std::locale("en_US.UTF-8"),
            "${:L}.{}",
            this->dollars(),
            this->cents()
        );
    }

    static void from_string(USD& self, std::string_view const str) {
        if(str.empty()) throw std::runtime_error{"Empty string passed to USD#from_string"};
        std::size_t start_dollars = 0;
        const std::size_t len = str.length();
        if(str.at(0) == '$') {
            start_dollars = 1;
        }
        std::size_t period_pos = str.find('.');
        period_pos = (period_pos == std::string_view::npos) ? len : period_pos;
        storage dollars = 0;
        auto [ptr, ec] = std::from_chars(str.data() + start_dollars, str.data() + period_pos, dollars);
        if(ec != std::errc()) {
            throw std::runtime_error{fmt::format("String '{}' is not a valid USD amount", str)};
        }
        self.dollars(dollars);

        if(period_pos != len) {
            storage cents = 0;
            auto [cptr, cec] = std::from_chars(str.data() + period_pos, str.end(), cents);
            if(cec != std::errc()) {
                throw std::runtime_error{fmt::format("String '{}' is not a valid USD amount", str)};
            }
            self.cents(cents);
        }
    }
private:
    /** \brief Number that is split at the `DECIMAL_POS` digit into whole dollars and cents */
    storage m_dec;
};
