#pragma once

#include <optional>

/** 
 * \brief Internal structure stored inside an `Optional` that allows for easier template specialization 
 * for types that have an invariant meaning they should be `none`
 * \tparam T Template specialization for all types that do not already specialize `OptionalInternal`
 */
template<typename T>
struct OptionalInternal {
public:
    /** \brief If this value is present in `Internal` */
    inline constexpr bool is_some() const {
        return this->opt.has_value();
    }
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
    /**
     * \brief Construct a new Optional that 
     */
    template<typename... Args>
    Optional(Args&&... args) : m_opt(std::forward<Args>(args)...) {}
private:
    OptionalInternal<T> m_opt;
};
