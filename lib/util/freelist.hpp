#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>
#include <concepts>
#include <assert.h>

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
        
    /**
     * \brief Elements in the `FreeList` may be either real elements or a single index pointing to the next 
     * element that is free, forming a kind of second linked list that records data about what slots in the vector 
     * are empty
     */
    using ListElem = std::variant<T, Next>;
public:
    FreeList() = default;
    FreeList(const FreeList<T>& other) = default;
    FreeList(FreeList<T>&& other) = default;
    FreeList<T>& operator=(FreeList<T>&& other) = default;
    FreeList<T>& operator=(const FreeList<T>& other) = default;
    
    /** \brief Get the number of free slots in this list */
    constexpr size_type free_slots() const {
        size_type count = 0;
        size_type next = this->free;
        while(next != npos) {
            count += 1;
            assert(next < this->m_vec.size());
            next = std::get<Next>(this->m_vec[next]).next;
        }
        return count;
    }
    
    /**
     * \brief Get the element at the given position, if the element at `pos` has already been freed this is UB
     */
    inline constexpr reference at(size_type pos) { assert(pos < this->m_vec.size()); return std::get<T>(this->m_vec[pos]); }
    inline constexpr const_reference at(size_type pos) const { assert(pos < this->m_vec.size()); return std::get<T>(this->m_vec[pos]); }
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
     * \return Index of the added element
     */
    template<typename... Args>
    requires(std::constructible_from<T, Args...>)
    size_type emplace(Args&&... args) {
        if(this->free != npos) {
            size_t free_pos = this->free;
            assert(free_pos < this->size());
            this->free = std::get<Next>(this->m_vec[this->free]).next;
            //new (&this->m_vec[free_pos]) T(std::forward<Args>(args)...);
            this->m_vec[free_pos].template emplace<T>(std::forward<Args>(args)...);
            return free_pos;
        } else {
            this->m_vec.push_back(ListElem{std::in_place_type<T>, std::forward<Args>(args)...});
            return this->m_vec.size() - 1;
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
        assert(pos < this->m_vec.size());
        std::visit(_detail::Visitor {
                [this, pos](T&) { this->m_vec[pos].template emplace<Next>(Next(this->free)); },
                [](auto) { throw std::runtime_error{"Attempt to erase element from FreeList twice"}; }
            },
            this->m_vec[pos]
        );
        this->free = pos;
    }
   
    /** \brief Iterator over all occupied slots in a `FreeList` */
    struct Iterator {
    public:
        using Iter = typename std::vector<ListElem>::iterator;
        using iterator_category = typename std::iterator_traits<Iter>::iterator_category;
        using difference_type = typename std::iterator_traits<Iter>::difference_type;
        using value_type = T;
        using pointer = T*; 
        using reference = T&;

        constexpr Iterator(Iter const& iter, Iter const& end) : m_iter{iter}, m_end{end} {}
        constexpr reference operator*() const { return std::get<T>(*this->m_iter); }
        constexpr pointer operator->() const { return std::addressof(std::get<T>(*this->m_iter)); }
        constexpr Iterator& operator++() {
            Iter current = this->m_iter;
            while(current != this->m_end && std::visit(_detail::Visitor {
                    [](T&) { return false; },
                    [](auto) { return true; }
                }, *current
            )) {
                    current++;
            }
            this->m_iter = current;
            return *this;
        }
        constexpr Iterator& operator++(int) const {
            Iterator tmp{this->m_iter, this->m_end};
            ++(*this);
            return tmp;
        }

        constexpr inline bool operator==(Iterator const&) const = default;
        constexpr inline bool operator!=(Iterator const&) const = default;
    
        /** \brief Get the index of this iterator in the `FreeList` */
        constexpr inline size_type idx() const noexcept {
            return this->m_iter;
        }
    private:
        Iter m_iter;
        Iter m_end;
    };



    /** \brief Constant iterator over all occupied slots in a `FreeList` */
    struct ConstIterator {
    public:
        using Iter = typename std::vector<ListElem>::const_iterator;
        using iterator_category = typename std::iterator_traits<Iter>::iterator_category;
        using difference_type = typename std::iterator_traits<Iter>::difference_type;
        using value_type = T;
        using pointer = T const *;
        using reference = T const&;

        constexpr ConstIterator(Iter const& iter, Iter const& end) : m_iter{iter}, m_end{end} {}
        constexpr reference operator*() const { return std::get<T>(*this->m_iter); }
        constexpr pointer operator->() const { return std::addressof(std::get<T>(*this->m_iter)); }
        constexpr Iterator& operator++() {
            Iter current = this->m_iter;
            while(current != this->m_end && std::visit(_detail::Visitor {
                    [](T&) { return false; },
                    [](auto) { return true; }
                }, *current
            )) {
                    current++;
            }
            this->m_iter = current;
            return *this;
        }
        constexpr ConstIterator& operator++(int) const {
            ConstIterator tmp{this->m_iter, this->m_end};
            ++(*this);
            return tmp;
        }
    
        /** \brief Get the index of this iterator in the `FreeList` */
        constexpr inline size_type idx() const noexcept {
            return this->m_iter;
        }

        constexpr inline bool operator==(ConstIterator const&) const = default;
        constexpr inline bool operator!=(ConstIterator const&) const = default;
    private:
        Iter m_iter;
        Iter m_end;
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

    inline constexpr iterator begin() noexcept { return Iterator{this->m_vec.begin(), this->m_vec.end()}; }
    inline constexpr iterator end() noexcept { return Iterator{this->m_vec.end(), this->m_vec.end()}; }
    inline constexpr const_iterator begin() const noexcept { return ConstIterator{this->m_vec.cbegin(), this->m_vec.cend()}; }
    inline constexpr const_iterator end() const noexcept { return ConstIterator{this->m_vec.cend(), this->m_vec.cend()}; }
    inline constexpr const_iterator cbegin() const noexcept { return ConstIterator{this->m_vec.cbegin(), this->m_vec.cend()}; }
    inline constexpr const_iterator cend() const noexcept { return ConstIterator{this->m_vec.cend(), this->m_vec.cend()}; }


    ~FreeList() = default;
private:     
    /** \brief Backing container of the list */
    std::vector<ListElem> m_vec;
    /** \brief Index of the first free element */
    size_type free{npos};
};
