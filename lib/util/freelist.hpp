#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>
#include <concepts>

namespace _detail {
template<typename... Ts>
struct Visitor : Ts... { using Ts::operator()...; };
}

/**
 * \brief Linked list data structure that allows removal of elements while preserving indices into the list
 */
template<typename T>
class FreeList {
public:
    using value = T;
    using reference = T&;
    using const_reference = T const&;
    
    using size_type = std::uint32_t;
    /** \brief An index value reserved for indicating an invalid index */
    static constexpr const size_type npos = std::numeric_limits<size_type>::max();

private:

    /** \brief Extra struct for using an std::variant with a separate type to T */
    struct Next { size_type next; };

public:
    FreeList() = default;
    FreeList(const FreeList& other) = default;
    FreeList(FreeList&& other) = default;
    
    /** \brief Get the number of free slots in this list */
    constexpr size_type free_slots() const {
        size_type count = 0;
        size_type next = this->free;
        while(next != npos) {
            count += 1;
            next = std::get<Next>(this->m_vec[next]).next;
        }
        return count;
    }
    
    /**
     * \brief Get the element at the given position, if the element at `pos` has already been freed this is UB
     */
    inline constexpr reference at(size_type pos) { return this->m_vec[pos].data; }
    inline constexpr const_reference at(size_type pos) const { return this->m_vec[pos].data; }
    inline constexpr reference operator[](size_type pos) { return this->at(pos); }
    inline constexpr const_reference operator[](size_type pos) const { return this->at(pos); }
        
    /**
     * \brief Get the number of elements in this `FreeList`, *not* the size including free slots
     */
    inline constexpr size_type size() const { return this->m_vec.size() - this->free_slots(); }
    
    /**
     * \brief Construct an instance of `T` in place from the given arguments
     * \tparam Args Argument types that `T` can be constructed from
     * \param args Argument values to construct an instance of `T` with
     * \return A reference to the added element
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    reference emplace(Args&&... args) {
        if(this->free != npos) {
            size_t free_pos = this->free;
            this->free = std::get<Next>(this->m_vec[this->free]).next;
            new (&this->m_vec[free_pos]) T(std::forward<Args>(args)...);
            return std::get<T>(this->m_vec[free_pos]);
        } else {
            return std::get<T>(this->m_vec.emplace_back(std::forward<Args>(args)...));
        }
    }
    /** \brief Copy the given value into this list, returning a reference to the inserted element */
    inline constexpr reference insert(const T& v) { return this->emplace(v); }
    /** \brief Move the given value into this list, returning a reference to the inserted element */
    inline constexpr reference insert(const T&& v) { return this->emplace(std::move(v)); }
    
    /**
     * \brief Remove the element at the given position 
     * \throws std::runtime_error if the element at `pos` was already deleted
     */
    inline void erase(size_type pos) {
        std::visit(_detail::Visitor {
                [this, pos](T&) { this->m_vec[pos].emplace(Next(this->free)); },
                [](auto) { throw std::runtime_error{"Attempt to erase element from FreeList twice"}; }
            },
            this->m_vec[pos]
        );
        this->m_vec[pos].next = this->free;
        this->free = pos;
    }

    ~FreeList() = default;
private:

    /**
     * \brief Elements in the `FreeList` may be either real elements or a single index pointing to the next 
     * element that is free, forming a kind of second linked list that records data about what slots in the vector 
     * are empty
     */
    using ListElem = std::variant<T, Next>;
      
    /** \brief Backing container of the list */
    std::vector<ListElem> m_vec;
    /** \brief Index of the first free element */
    size_type free{npos};
};
