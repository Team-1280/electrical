#pragma once

#include <util/optional.hpp>

/**
 * \brief Class emulating an `std::vector that must always have one element`
 * \implements Noneable
 */
template<typename T>
class SingleVec {
public: 
    SingleVec() = default;
    
    /**
     * \brief Construct a new `SingleVec` that contains the given first element
     * \tparam U Type of the first element, parameterized to allow forwarding references
     * \param first THe first element of this `SingleVec`
     */
    template<typename U = T>
    constexpr SingleVec(U&& first) : m_elems{} {
        this->m_elems.push_back(std::forward<U>(first));
    }
    
    /** \brief Move construct this `SingleVec` from another `SingleVec` */
    SingleVec(SingleVec<T>&& other) : m_elems{std::move(other.m_elems)} {}
    SingleVec(SingleVec<T> const& other) : m_elems{other.m_elems} {}
   
    SingleVec<T>& operator=(SingleVec<T>&& other) {
        this->m_elems = std::move(other.m_elems);
        return *this;
    }
    SingleVec<T>& operator=(SingleVec<T> const& other) {
        this->m_elems = other.m_elems;
        return *this;
    }

    /**
     * \brief Append `elem` to this `SingleVec`
     */
    template<typename U = T>
    inline constexpr void push_back(U&& elem) {
        this->m_elems.push_back(std::forward<U>(elem));
    }

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    inline constexpr iterator begin() { return this->m_elems.begin(); }
    inline constexpr iterator end() { return this->m_elems.end(); }
    inline constexpr const_iterator begin() const { return this->m_elems.begin(); }
    inline constexpr const_iterator end() const { return this->m_elems.end(); }
    
    /** \brief Convert this std::vector to JSON if `T` can be serialized as json */
    json to_json() const requires(ser::JsonSerializable<T>) {
        return this->m_elems;
    }
    
    /** \brief Deserialize this `SingleVec` from a JSON value if `T` supports the operation */
    static void from_json(SingleVec<T>& self, json const& json) requires(ser::JsonSerializable<T>) {
        json.get_to<std::vector<T>>(self.m_elems);
    }

    /** 
     * \brief For implementing `Noneable`, create an invalid `SingleVec` that can be treated as a 
     * none variant by an `Optional`
     */
    constexpr void make_none() { this->m_elems.clear(); }
    /** \brief For `Noneable` implementation, check if this `SingleVec` is invalid */
    constexpr inline bool is_none() const noexcept { return this->m_elems.empty(); }
private:
    /** \brief Vector that must contain one element */
    std::vector<T> m_elems;
};
