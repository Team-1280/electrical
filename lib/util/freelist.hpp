#pragma once

#include "util/stackvec.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>


/**
 * \brief Linked list data structure that allows removal of elements while preserving indices into the list
 */
template<typename T>
class FreeList {
public:
    using size_type = std::uint32_t;
    /** \brief An index value reserved for indicating an invalid index */
    static constexpr const size_type npos = std::numeric_limits<size_type>::max();
    
    FreeList() = default;
    FreeList(const FreeList& other) = default;
    FreeList(FreeList&& other) = default;
    
    /** \brief Get the number of free slots in this list */
    constexpr size_type free_slots() const {
        size_type count = 0;
        size_type next = this->free;
        while(next != npos) {
            count += 1;
            next = this->m_vec[next].next;
        }
        return count;
    }
        
    /**
     * \brief Get the number of elements in this `FreeList`, *not* the size including free slots
     */
    inline constexpr size_type size() const { return this->m_vec.size() - this->free_slots(); }
    
    /**
     * \brief Remove the element at the given position 
     * **WARNING:** Erasing the same index twice is undefined behavior
     */
    inline void erase(size_type pos) {
        this->m_vec[pos].~T();
        this->m_vec[pos].next = this->free;
        this->free = pos;
    }

    ~FreeList() {
        std::unordered_set<size_type> empty{};
        size_type next = this->free;
        while(next != npos) {
            empty.emplace(next);
            next = this->m_vec[next].next;
        }
        
        for(size_type i = this->m_vec.size() - 1; i >= 0; i--) {
            if(!empty.contains(i)) {
                this->m_vec[i].~T();
            }
        }
    }
private:
    /**
     * \brief Elements in the `FreeList` may be either real elements or a single index pointing to the next 
     * element that is free, forming a kind of second linked list that records data about what slots in the vector 
     * are empty
     */
    union ListElem {
        /** An element of type T */
        T data;
        /** An index to the next free element, or `npos` */
        size_type next;
    };

    /** \brief Backing container of the list */
    std::vector<ListElem> m_vec;
    /** \brief Index of the first free element */
    size_type free{npos};
};
