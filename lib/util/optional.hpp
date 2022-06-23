#pragma once

#include <functional>
#include <memory>
#include <optional>

/** 
 * \brief Internal structure stored inside an `Optional` that allows for easier template specialization 
 * for types that have an invariant meaning they should be `none`
 * \tparam T Template specialization for all types that do not already specialize `OptionalInternal`
 */
template<typename T>
struct OptionalInternal {
public:
    /** \brief Create an empty optional */
    OptionalInternal() : opt{} {}

    /** 
     * \brief Construct this optional internal from the given arguments, constructing a value in place
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    OptionalInternal(Args&&... args) : opt{std::forward<Args>(args)...} {}

    /** \brief If this value is present in `Internal` */
    inline constexpr bool is_some() const noexcept {
        return this->opt.has_value();
    }
    
    /** 
     * \brief Get a pointer to the value stored in this `OptionalInternal`, or a nullptr 
     */
    inline T& get() & { return this->opt.value(); }
    inline T const& get() const& { return this->opt.value(); }
    inline T&& get() && { return this->opt.value(); }
    
    /** \brief Erase the value, if any, currently held in this `OptionalInternal` */
    inline void reset() noexcept { this->opt.reset(); }
    /**
     * \brief Construct a value inside this `OptionalInternal` using the given arguments 
     * \tparam Args Argument types that `T` must be constructible from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    inline void emplace(Args&&... args) { this->opt.emplace(std::forward<Args>(args)...); }

    static void from_json(OptionalInternal<T>& self, json const& json) requires(ser::JsonSerializable<T>) {
        if(json.is_null()) { self.reset(); }
        else {
            self.emplace();
            json.get_to<T>(self.get());
        }
    }

    json to_json() const requires(ser::JsonSerializable<T>) { return this->is_some() ? this->get().to_json() : nullptr; }
private:
    std::optional<T> opt;
};


/**
 * \brief Wrapper around an `std::optional` that allows easier template specialization for types that 
 * have an invariant that means they should be considered none, and allows for JSON serialization
 */
template<typename T>
class Optional {
public:
    /** \brief Create a new empty Optional */
    inline constexpr Optional() : m_opt{} {}

    /**
     * \brief Construct a new Optional constructed from the given arguments
     * \tparams Args Argument types that `T` can be constructed from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    Optional(Args&&... args) : m_opt(std::forward<Args>(args)...) {}
    
    /** \brief Check if this Optional contains a value */
    constexpr inline bool is_some() const {
        return this->m_opt.is_some();
    }
    
    template<std::invocable<T const&> IfSome>
    constexpr Optional<std::invoke_result_t<IfSome, T const&>> map(IfSome&& if_some) const& {
        return this->m_opt.is_some() ? std::invoke(std::forward<IfSome>(if_some), this->m_opt.get()) : Optional();
    } 

    inline void to_json() const requires(ser::JsonSerializable<OptionalInternal<T>>) { return this->m_opt.to_json(); }
    inline static void from_json(Optional<T>& self, json const& json) requires(ser::JsonSerializable<OptionalInternal<T>>) { OptionalInternal<T>::from_json(self.m_opt, json); }
private:
    OptionalInternal<T> m_opt;
};
