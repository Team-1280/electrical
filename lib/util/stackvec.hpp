#pragma once

#include <array>
#include <cstdint>
#include <concepts>
#include <cstdlib>

/**
 * \brief Vector structure that stores up to `MAX_STACK` elements on the stack before allocating heap space
 * \tparam T Type of element to store in this `StackVec`
 * \tparam MAX_STACK Maximum number of elements to store on the stack, defaults to 1024 bytes
 */
template<typename T, std::size_t MAX_STACK = 1024 / sizeof(T)>
class StackVec {
public:
    using size_type = std::uint32_t;
    using reference = T&;
    using const_reference = T const&;
    
    StackVec() = default;
    
    /**
     * \brief Add an element to the end of this vector
     * \param val Value to append by calling the copy constructor
     */
    constexpr void push_back(const T& val) requires(std::copy_constructible<T>){
        if(this->m_len >= MAX_STACK) {
            if(this->m_len >= this->m_cap) {
                //Either allocate another MAX_STACK elements on the heap or double the capacity
                this->m_cap = (this->m_cap == 0) ? MAX_STACK : this->m_cap * 2;
                this->m_heap = std::realloc(this->m_heap, sizeof(T) * this->m_cap);
            }
            new (this->m_heap + this->m_len) T(val);
        } else {
            new (&this->m_stack[this->m_len]) T(val);
        }
        this->m_len += 1;
    }
    
    /**
     *
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr reference emplace_back(Args&&... args) {

    }
    
    /**
     * \brief Get the element at the given position
     * \param pos Must be less than the length of this vector
     */
    inline constexpr reference at(size_type pos) {
        assert(pos < this->m_len);
        return (pos >= MAX_STACK) ? this->m_heap[pos - MAX_STACK] : this->m_stack[pos];
    }
    inline constexpr const_reference at(size_type pos) const {
        assert(pos < this->m_len);
        return (pos >= MAX_STACK) ? this->m_heap[pos - MAX_STACK] : this->m_stack[pos];
    }

    inline constexpr reference operator[](size_type pos) { return this->at(pos); }
    inline constexpr const_reference operator[](size_type pos) const { return this->at(pos); }
    
    /** \brief Check if this vector contains no elements */
    inline constexpr bool empty() const noexcept { return this->m_len == 0; }
    
    /** \brief Get the number of elements in this vector */
    inline constexpr size_type size() const noexcept { return this->m_len; }
    
    /** \brief Return true if this `StackVec` has begun allocating elements on the heap */
    inline constexpr bool is_heap() const noexcept { return this->m_len >= MAX_STACK; }

    ~StackVec() {
        size_type i = 0;
        while(i < MAX_STACK && i < this->m_len) {
            this->m_stack[i].~T();
        }
        i = 0;
        while(i < this->m_len - MAX_STACK) {
            this->m_heap[i].~T();
        }
        std::free(this->m_heap);
    }
private:
    /** \brief Elements currently stored on the stack */
    T m_stack[MAX_STACK];
    /** \brief Capacity of the vector, i.e amount of space allocated on the heap */
    size_type m_cap{0};
    /** \brief Pointer to extra heap-allocated space */
    T* m_heap{nullptr};
    /** \brief Length of the vector, includes stack space */
    size_type m_len{0};
};
