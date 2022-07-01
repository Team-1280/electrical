#pragma once

#include <cstdint>
#include <concepts>
#include <cstdlib>
#include <iterator>

/**
 * \brief Vector structure that stores up to `MAX_STACK` elements on the stack before allocating heap space
 * \tparam T Type of element to store in this `StackVec`
 * \tparam MAX_STACK Maximum number of elements to store on the stack, defaults to 1024 bytes
 */
template<typename T, std::size_t MAX_STACK = 1024 / sizeof(T)>
class StackVec {
public:
    using value_type = T;
    using size_type = std::uint32_t;
    using reference = T&;
    using const_reference = T const&;
    
    /** \brief Iterator over elements of a StackVec */
    struct Iterator {
    public:
        /** \brief Construct a new Iterator over elements of a `StackVec`, starting with the given index */
        Iterator(StackVec<T>& ref, size_type idx = 0) : m_vec{ref}, m_idx{idx} {}
        Iterator(const Iterator& other) = default;
    

        using iterator_category = std::forward_iterator_tag;
        using difference_type = ssize_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        reference operator*() const { return this->m_vec[this->m_idx]; }
        pointer operator->() const { return &this->m_vec[this->m_idx]; }
        Iterator& operator++() { this->m_idx += 1; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; this->operator++(); return tmp; }

        bool operator==(const Iterator& other) const = default;
        bool operator!=(const Iterator& other) const = default;


    private:
        /** \brief Index of the currently accessed element */
        size_type m_idx{0};
        /** \brief Reference to the StackVec that we access */
        StackVec<T>& m_vec;
    };

    /** \brief Iterator over constant elements of a StackVec */
    struct ConstIterator {
    public:
        /** \brief Construct a new Iterator over elements of a `StackVec`, starting with the given index */
        ConstIterator(StackVec<T>& ref, size_type idx = 0) : m_vec{ref}, m_idx{idx} {}
        ConstIterator(const ConstIterator& other) = default;
    

        using iterator_category = std::input_iterator_tag;
        using difference_type = ssize_t;
        using value_type = T;
        using pointer = const T *;
        using reference = T const&;

        reference operator*() const { return this->m_vec[this->m_idx]; }
        pointer operator->() const { return &this->m_vec[this->m_idx]; }
        ConstIterator& operator++() { this->m_idx += 1; return *this; }
        ConstIterator operator++(int) { ConstIterator tmp = *this; this->operator++(); return tmp; }

        bool operator==(const ConstIterator& other) const = default;
        bool operator!=(const ConstIterator& other) const = default;
    private:
        /** \brief Index of the currently accessed element */
        size_type m_idx{0};
        /** \brief Reference to the StackVec that we access */
        StackVec<T> const& m_vec;
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;
    
    StackVec() = default;
    StackVec(const StackVec& other) = default;
    StackVec(StackVec&& other) = default;
    
    /**
     * \brief Add an element to the end of this vector
     * \param val Value to append by calling the copy constructor
     */
    inline constexpr reference push_back(const T& val) requires(std::copy_constructible<T>){
        return this->emplace_back(val); 
    }
    /**
     * \brief Move an instance of T into this vector
     */
    inline constexpr reference push_back(T&& val) requires(std::move_constructible<T>) {
        return this->emplace_back(std::move(val));
    }
    
    /**
     * \brief Push an element to the end of this vector by constructing it from the given arguments
     * \tparam Args Argument types that T will be constructed from
     * \param Args Argument values to construct an instance of T from
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    constexpr reference emplace_back(Args&&... args) {
        if(this->m_len >= MAX_STACK) {
            if(this->m_len >= this->m_cap) {
                //Either allocate another MAX_STACK elements on the heap or double the capacity
                this->m_cap = (this->m_cap == 0) ? MAX_STACK : this->m_cap * 2;
                this->m_heap = static_cast<T*>(std::realloc(this->m_heap, sizeof(T) * this->m_cap));
            }
            new (this->m_heap + this->m_len) T(std::forward<Args>(args)...);
            this->m_len += 1;
            return this->m_heap[this->m_len - 1];
        } else {
            new (&this->m_stack[this->m_len]) T(std::forward<Args>(args)...);
            this->m_len += 1;
            return this->m_stack[this->m_len - 1];
        }
    }
    
    /**
     * \brief Remove the last element of this vector
     */
    constexpr void pop_back() {
        const size_type removed = this->m_len - 1;
        if(removed >= MAX_STACK) {
            this->m_heap[removed].~T();
        } else {
            this->m_stack[removed].~T();
        }
        this->m_len = removed;
    }
    
    /**
     * \brief Clear all elements from this `StackVec`, invalidating all iterators, references, and pointers
     * into this vector
     */
    constexpr void clear() {
        size_type i = 0;
        while(i < MAX_STACK && i < this->m_len) {
            this->m_stack[i].~T();
        }
        i = 0;
        while(i < this->m_len - MAX_STACK) {
            this->m_heap[i].~T();
        }
        this->m_len = 0;
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

    iterator begin() { return iterator(*this); }
    iterator end() { return iterator(*this, this->m_len); }
    const_iterator begin() const { return const_iterator(*this); }
    const_iterator end() const { return const_iterator(*this, this->m_len); }

    ~StackVec() {
        this->clear();
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
