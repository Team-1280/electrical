#pragma once

#include <string>
#include <tuple>
#include <concepts>
#include <vector>

template<typename T>
struct LazyResourceLoader;

/**
 * \brief Concept for lazily-loaded immutable data specifying how the data will be loaded,
 * requiring a template specialization of `LazyResourceLoader`
 */
template<typename T>
concept LazyResource = requires {
    sizeof(LazyResourceLoader<T>) != 0;
};


/**
 * \brief Structure wrapping a single string, composed of multiple ID segments separated by the '.' character
 */
struct Id {
public:
    /** Iterator over the segments of an `Id` */
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::string_view;
        using pointer = value_type*;
        using reference = value_type;

        Iterator(Id const& super) : m_super{super}, m_pos{0} {}
        Iterator(Id const& super, std::size_t pos) : m_super{super}, m_pos{pos} {}

        Iterator& operator++();
        Iterator operator++(int);
        /** Access the currently held ID segment */
        reference operator*() const;

        bool operator==(Iterator other) const;
        bool operator!=(Iterator other) const;

    private:
        /** Parent ID that the iterator references */
        Id const& m_super;
        /** Position in the parent's string that we are at */
        std::size_t m_pos;
        
        friend struct Id;
    };
    
    /** Return an iterator pointing to the first element of this ID */
    Iterator begin() const;
    /** Return an iterator pointing to one past the last element of this ID */
    Iterator end() const;

    /** Initialize this ID with no segments */
    inline Id() : m_string{}, m_dots{} {}
    /** Move an owned string into this ID */
    Id(std::string&& str);
    /** Copy a string to this ID */
    Id(const std::string& str);
private:
    /** String that contains all data of the `Id`, not implemented as a linked list because memory fragmentation is bad */
    std::string m_string;
    /** Location in the backing string of all period characters */
    std::vector<std::size_t> m_dots;

    friend class Id::Iterator;
};


/**
 * \brief Class containing many `LazyResource`s with methods to load them from folders
 */
template<LazyResource... Resources> 
class LazyResourceStore {
public:
    /** \brief Construct this resource store, performs no I/O operations */
    LazyResourceStore() : m_res{} {}
private:
    std::tuple<Resources...> m_res;
};
