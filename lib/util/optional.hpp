#pragma once

#include "ser/ser.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
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
     * \brief Get a pointer to the value stored in this `OptionalInternal`, or a nullptr 
     */
    constexpr inline T& get() & { return this->opt.value(); }
    constexpr inline T const& get() const& { return static_cast<std::optional<T> const&>(this->opt).value(); }
    constexpr inline T&& get() && { return std::move(this->opt).value(); }
    constexpr inline T const&& get() const&& { return static_cast<std::optional<T> const&&>(std::move(this->opt)).value(); }
    
    /** \brief Erase the value, if any, currently held in this `OptionalInternal` */
    constexpr inline void reset() noexcept { this->opt.reset(); }
    /**
     * \brief Construct a value inside this `OptionalInternal` using the given arguments 
     * \tparam Args Argument types that `T` must be constructible from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr inline void emplace(Args&&... args) { this->opt.emplace(std::forward<Args>(args)...); }

    static void from_json(OptionalInternal<T>& self, json const& json) requires(ser::JsonSerializable<T>) {
        if(json.is_null()) { self.reset(); }
        else {
            self.emplace();
            json.get_to<T>(self.get());
        }
    }

    json to_json() const requires(ser::JsonSerializable<T>) { return this->has_value() ? this->get().to_json() : nullptr; }

    virtual ~OptionalInternal() = default;
private:
    std::optional<T> opt;
};


/**
 * \brief Wrapper around an `std::optional` that allows easier template specialization for types that 
 * have an invariant that means they should be considered none, and allows for JSON serialization
 */
template<typename T>
class Optional final {
public:
    /** \brief Create a new empty Optional */
    inline constexpr Optional() : m_opt{} {}
    
    /** Move the given value into this optional */
    template<typename U = T>
    constexpr inline Optional(U&& val) requires(std::constructible_from<T, U>) : m_opt{} {
        this->emplace(std::forward<U>(val));
    }
    
    /** \brief Move the optional of an optionally different type into this Optional */
    template<typename U = T>
    constexpr Optional(Optional<U>&& val) requires(std::constructible_from<T, U&&>) : m_opt{} {
        if(val.has_value()) {
            this->template emplace<U&&>(std::move(val).unwrap());   
        }
    }
    
    /** \brief Copy-construct this `Optional` from another `Optional` containing an optionally different type */
    template<typename U = T>
    constexpr Optional(const Optional<U>& other) requires(std::constructible_from<T, U const&>) : m_opt{} {
        if(other.has_value()) {
            this->emplace<T const&>(other.unwrap());
        }
    }
    
    /** \brief Clear the value from this `Optional` */
    constexpr inline Optional<T>& operator=(std::nullopt_t) noexcept {
        this->reset();
        return *this;
    }
    
    /**
     * \brief Assign the given value to this Optional
     * \param val The value to emplace into this Optional
     */
    template<typename U = T>
    requires(std::constructible_from<T, U>)
    constexpr Optional<T>& operator=(U&& val) noexcept {
        this->emplace(std::forward<U>(val));
        return *this;
    }

    /**
     * \brief Construct a new Optional constructed from the given arguments
     * \tparams Args Argument types that `T` can be constructed from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr explicit Optional(std::in_place_t in_place, Args&&... args) : m_opt(in_place, std::forward<Args>(args)...) {}
    
    /** \brief Check if this Optional contains a value */
    constexpr inline bool has_value() const noexcept {
        return this->m_opt.has_value();
    }
    
    /** Implicitly check if this Optional contains a value by converting it to a boolean */
    constexpr inline operator bool() const noexcept { return this->has_value(); }
    
    /** Get the value contained in this `Optional` or throw an exception / panic */
    constexpr inline T& unwrap() & { return this->m_opt.get(); }
    constexpr inline T const& unwrap() const& { return static_cast<OptionalInternal<T> const&>(this->m_opt).get(); }
    constexpr inline T&& unwrap() && { return std::move(this->m_opt).get(); }
    constexpr inline T const&& unwrap() const&& { return static_cast<OptionalInternal<T> const&&>(std::move(this->m_opt)).get(); }
    
    /**
     * \brief Construct a value of type `T` in place in this Optional, replacing any result that used to be contained
     * \tparam Args Argument types that `T` can be constructed from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr void emplace(Args&&... args) {
        this->m_opt.emplace(std::forward<Args>(args)...);
    }
    
    /** \brief Remove the value from this `Optional` */
    constexpr inline void reset() noexcept {
        this->m_opt.reset();
    }

    /**
     * \brief Apply the given function to this Optional and return a new Optional containing its result
     * \tparam IfSome type of function to run on the value contained in this `Optional` if it has a value
     * \return An `Optional` containing either the result of the function or no value
     */
    template<std::invocable<T const&> IfSome>
    constexpr Optional<std::invoke_result_t<IfSome, T const&>> map(IfSome&& if_some) const& {
        return this->m_opt.has_value() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional{};
    } 

    inline void to_json() const requires(ser::JsonSerializable<OptionalInternal<T>>) { return this->m_opt.to_json(); }
    inline static void from_json(Optional<T>& self, json const& json) requires(ser::JsonSerializable<OptionalInternal<T>>) { OptionalInternal<T>::from_json(self.m_opt, json); }
private:
    OptionalInternal<T> m_opt;
};
