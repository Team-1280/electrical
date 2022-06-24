#pragma once

#include "ser/ser.hpp"
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
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

    virtual ~OptionalInternal() = default;
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
    constexpr inline T& get() & { return this->opt; }
    constexpr inline T const& get() const& { return this->opt; }
    constexpr inline T&& get() && { return std::move(this->opt); }
    constexpr inline T const&& get() const&& { return std::move(this->opt); }
    
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

    virtual ~OptionalInternal() = default;
private:
    T opt;
};


/**
 * \brief Wrapper around an `std::optional` that allows easier template specialization for types that 
 * have an invariant that means they should be considered none, and allows for JSON serialization.
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
    
    /** \brief Unwrap this `Optional`, or `other` if this contains no value */
    constexpr inline T& unwrap_or(T& other) & noexcept { return this->has_value() ? this->unwrap() : other; }
    constexpr inline T const& unwrap_or(T const& other) const& noexcept { return this->has_value() ? this->unwrap() : other; }
    constexpr inline T&& unwrap_or(T&& other) && noexcept { return this->has_value() ? this->unwrap() : other; }
    constexpr inline T const&& unwrap_or(T const&& other) const&& noexcept { return this->has_value() ? this->unwrap() : other; }

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
        return this->m_opt.has_value() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional<std::invoke_result_t<IfSome, T const&>>{};
    }

    /**
     * \brief Apply the given function to this Optional and return a new Optional containing its result
     * \tparam IfSome type of function to run on the value contained in this `Optional` if it has a value
     * \return An `Optional` containing either the result of the function or no value
     */
    template<std::invocable<T&> IfSome>
    constexpr Optional<std::invoke_result_t<IfSome, T&>> map(IfSome&& if_some) & {
        return this->m_opt.has_value() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional<std::invoke_result_t<IfSome, T&>>{};
    }

    /**
     * \brief Apply the given function to this Optional and return a new Optional containing its result
     * \tparam IfSome type of function to run on the value contained in this `Optional` if it has a value
     * \return An `Optional` containing either the result of the function or no value
     */
    template<std::invocable<T&&> IfSome>
    constexpr Optional<std::invoke_result_t<IfSome, T&&>> map(IfSome&& if_some) && {
        return this->m_opt.has_value() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional<std::invoke_result_t<IfSome, T&&>>{};
    }

    /**
     * \brief Apply the given function to this Optional and return a new Optional containing its result
     * \tparam IfSome type of function to run on the value contained in this `Optional` if it has a value
     * \return An `Optional` containing either the result of the function or no value
     */
    template<std::invocable<T const&&> IfSome>
    constexpr Optional<std::invoke_result_t<IfSome, T const&&>> map(IfSome&& if_some) const&& {
        return this->m_opt.has_value() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional<std::invoke_result_t<IfSome, T const&&>>{};
    }

    static void from_json(Optional<T>& self, json const& json) requires(ser::JsonSerializable<T>) {
        if(json.is_null()) { self.reset(); }
        else {
            self.emplace();
            json.get_to<T>(self.unwrap());
        }
    }

    json to_json() const requires(ser::JsonSerializable<T>) { return this->has_value() ? this->get().to_json() : nullptr; }

    inline std::string to_string() const requires(ser::StringSerializable<T>) { return this->map(T::to_string).unwrap_or(std::string{}); }
    inline static void from_string(Optional<T>& self, std::string_view str) requires(ser::StringSerializable<T>) {
        self.emplace();
        T::from_string(self.unwrap(), str);
    }
    
    /**
     * \brief Check if two `Optional`s are equal
     * \return true if both `Optional`s have no value, or if they both have a value and the values are equal
     */
    constexpr bool operator==(const Optional<T>& other) const requires requires(T v) {
        {v == v} -> std::convertible_to<bool>;
    } {
        return (this->has_value() && other.has_value()) ? this->get() == other.get() : 
            (!this->has_value() && !other.has_value()) ? true : false;
    }

    /**
     * \brief Check if two `Optional`s are not equal
     * \return true if both `Optional`s have no value, or if they both have a value and the values are not equal
     */
    constexpr bool operator!=(const Optional<T>& other) const requires requires(T v) {
        {v != v} -> std::convertible_to<bool>;
    } {
        return (this->has_value() && other.has_value()) ? this->unwrap() != other.unwrap() : 
            (!this->has_value() && !other.has_value()) ? false : true;
    }
    
    /**
     * \brief If the type stored in an `Optional` can be iterated over, this type will contain either an
     * iterator over elements that `T` contains or an iterator that is always equal to the `end()` function
     */
    template<std::input_or_output_iterator Iter>
    struct Iterator final {
    public:
        using iterator_category = typename std::iterator_traits<Iter>::iterator_category;
        using difference_type = typename std::iterator_traits<Iter>::difference_type;
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using pointer = typename std::iterator_traits<Iter>::pointer;
        using reference = typename std::iterator_traits<Iter>::reference;
        
        inline constexpr Iterator() : m_internal{} {}
        constexpr Iterator(Iter i) : m_internal{i} {}
        constexpr Iterator(const Optional<Iter>& i) : m_internal{i} {}

        constexpr reference operator*() const { return this->m_internal.unwrap().operator*(); }
        constexpr pointer operator->() { return this->m_internal.unwrap().operator->(); }
        constexpr Iterator& operator++() {
            if(this->m_internal.has_value()) {
                this->m_internal.unwrap().operator++();
            }
            return *this;
        }
        constexpr Iterator operator++(int) {
            Iterator tmp{*this};
            this->operator++();
            return tmp;
        }

        constexpr bool operator==(const Iterator& other) const { return this->m_internal == other.m_internal; }
        constexpr bool operator!=(const Iterator& other) const { return this->m_internal != other.m_internal; }
    private:
        /** Internal iterator or none */
        Optional<Iter> m_internal;
    };
    
    /** \brief Get an `Iterator` over the elements contained in `T` if it has a value, or an empty iterator */
    constexpr auto begin() & requires requires(T v) {
        requires std::input_or_output_iterator<decltype(v.begin())>;
    } {
        return Iterator<decltype(std::declval<T&>().begin())>{this->map([](T& v) { return v.begin(); })};
    }


    /** \brief Get an `Iterator` over the elements contained in `T` pointing to the end of the collection if it has a value, or an empty iterator */
    constexpr auto end() & requires requires(T v) {
        requires std::input_or_output_iterator<decltype(v.end())>;
    } {
        return Iterator<decltype(std::declval<T&>().begin())>{this->map([](T& v) { return v.end(); })};
    }

    /** \brief Get an `Iterator` over the elements contained in `T` if it has a value, or an empty iterator */
    constexpr auto begin() const& requires requires(const T v) {
        requires std::input_or_output_iterator<decltype(v.begin())>;
    } {
        return Iterator<decltype(std::declval<T const&>().begin())>{this->map([](T const& v) { return v.begin(); })};
    }


    /** \brief Get an `Iterator` over the elements contained in `T` pointing to the end of the collection if it has a value, or an empty iterator */
    constexpr auto end() const& requires requires(const T v) {
        requires std::input_or_output_iterator<decltype(v.end())>;
    } {
        return Iterator<decltype(std::declval<T const&>().begin())>{this->map([](T const& v) { return v.end(); })};
    }

private:
    OptionalInternal<T> m_opt;
};
