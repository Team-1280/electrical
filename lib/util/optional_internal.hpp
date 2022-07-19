#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <utility>

/** 
 * \brief Internal structure stored inside an `Optional` that allows for easier template specialization 
 * for types that have an invariant meaning they should be `none`
 * \tparam T Template specialization for all types that do not already specialize `OptionalInternal`
 */
template<typename T>
struct OptionalInternal {
public:
    /** \brief Create an empty optional */
    constexpr OptionalInternal() : opt{} {}

    /** \brief If this value is present in `Internal` */
    inline constexpr bool has_value() const noexcept {
        return this->opt.has_value();
    }
    
    /** 
     * \brief Get the value stored in this `OptionalInternal`, or a nullptr 
     */
    constexpr inline T& unwrap_unchecked() & { return this->opt.operator*(); }
    constexpr inline T const& unwrap_unchecked() const& { return static_cast<std::optional<T> const&>(this->opt).operator*(); }
    constexpr inline T&& unwrap_unchecked() && { return std::move(this->opt).operator*(); }
    constexpr inline T const&& unwrap_unchecked() const&& { return static_cast<std::optional<T> const&&>(std::move(this->opt)).operator*(); }
    
    /** \brief Erase the value, if any, currently held in this `OptionalInternal` */
    constexpr inline void reset() noexcept { this->opt.reset(); }
    /**
     * \brief Construct a value inside this `OptionalInternal` using the given arguments 
     * \tparam Args Argument types that `T` must be constructible from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr inline void emplace(Args&&... args) { this->opt.emplace(std::forward<Args>(args)...); }

    ~OptionalInternal() = default;
private:
    std::optional<T> opt;
};


/**
 * \brief Concept specifying that a type `T` can be either a none or some value based on 
 * conditions stored in the instance of `T`, allowing for an optimization eliminating the overhead of an std::optional<T>
 */
template<typename T>
concept Noneable = requires(T v) {
    {v.is_none()} -> std::same_as<bool>;
    {v.make_none()};
    std::is_move_assignable_v<T>;
};

/**
 * \brief Structure wrapping a `T` with a known invariant value that allows existing
 * types to implement `Noneable` easily by specifying a value that the type will never be
 */
template<typename T, T INVARIANT>
requires requires(T v) {
    {v == v} -> std::convertible_to<bool>;
} struct Invariant {
public:
    /**
     * \brief Create a new `Invariant` implicitly from a value of the contained type
     */
    template<typename U = T>
    requires requires(U&& val) {
        std::constructible_from<T, U>;
    } inline constexpr Invariant(U&& val) : m_val{std::forward<U>(val)} {}
    
    /** \brief Check if this value is an invariant */
    constexpr bool is_none() const {
        return this->m_val == INVARIANT;
    }

    constexpr void make_none() noexcept {
        this->m_val = INVARIANT:
    }
private:
    T m_val;
};

/**
 * \brief Template specialization of `OptionalInternal` for types that satisfy the `Noneable` concept, used to
 * eliminate the overhead of an `std::optional`
 * \sa Noneable
 */
template<Noneable T>
struct OptionalInternal<T> {
public:
    /** \brief Create an empty optional */
    constexpr OptionalInternal() : opt{} {}

    /** \brief If this value is present in `Internal` */
    inline constexpr bool has_value() const noexcept {
        return !this->opt.is_none();
    }
    
    /** 
     * \brief Get a pointer to the value stored in this `OptionalInternal`, or a nullptr 
     */
    constexpr inline T& unwrap_unchecked() & { return this->opt; }
    constexpr inline T const& unwrap_unchecked() const& { return this->opt; }
    constexpr inline T&& unwrap_unchecked() && { return std::move(this->opt); }
    constexpr inline T const&& unwrap_unchecked() const&& { return std::move(this->opt); }
    
    /** \brief Erase the value, if any, currently held in this `OptionalInternal` */
    constexpr inline void reset() noexcept { this->opt.make_none(); }
    /**
     * \brief Construct a value inside this `OptionalInternal` using the given arguments 
     * \tparam Args Argument types that `T` must be constructible from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr inline void emplace(Args&&... args) {
        this->opt = T{std::forward<Args>(args)...};
    }

    ~OptionalInternal() = default;
private:
    T opt;
};
