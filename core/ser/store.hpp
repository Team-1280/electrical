#pragma once

#include "resource.hpp"
#include "ser.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <concepts>
#include <vector>

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
using Optional = std::optional<T>;

/**
 * \brief Concept ensuring that the parameter pack Other contains type T
 * \tparam T The type to search for in `Other`
 * \tparam Other Template type parameter pack to search for `T` in
 */
template<typename T, typename... Other>
concept Contains = (std::same_as<T, Other> || ...);

class LazyResourceStore;

/**
 * Structure containing a single std::size_t that is generated for every unique type that `id` is called with.
 * Effectively provides run-time type information for the LazyResourceStore class
 */
struct TypeId {
    template<typename T>
    static TypeId id() {
        static std::size_t ID = IDX++;
        return TypeId{ID};
    }
    
    /** \brief Create a new TypeId from a raw counter value */
    TypeId(std::size_t id) : m_id{id} {}
    TypeId() = delete;
    
    /** 
     * \brief Get the raw counter value of this ID
     */
    constexpr inline std::size_t val() const { return this->m_id; }

private:
    /** Static index that is incremented every time `id` is called with a new type */
    static std::size_t IDX;
    /** The counter value of this unique type id */
    std::size_t m_id;
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
    
    /** Get the string that backs this ID */
    inline constexpr std::string const& str() const { return this->m_string; }
private:
    /** String that contains all data of the `Id`, not implemented as a linked list because memory fragmentation is bad */
    std::string m_string;
    /** Location in the backing string of all period characters */
    std::vector<std::size_t> m_dots;

    friend class Id::Iterator;
};


/**
 * \brief Class that can load a type-erased value from a file, derive from this type 
 * to register a new loader. Note that nobody should derive from this class, instead see `LazyResourceLoader`, which gurantees type safety
 * \sa LazyResourceLoader
 */
class ErasedLazyResourceLoader {
public:
    virtual ~ErasedLazyResourceLoader() = 0;
    ErasedLazyResourceLoader() = delete;
private:
    /**
     * \brief Load a value from the given JSON value, using a `LazyResourceStore` to deserialize other values by ID
     * \return A type-erased shared pointer to the value that we will deserialize, **MUST** be of the same type as our
     * `id` method. If it is of a different type, Undefined Behavior **WILL** occur.
     *
     * \param json A deserialized JSON value to load from
     * \param store A lazy loading resource store that we can use to load values of other types
     */
    virtual Ref<void> load_untyped(
        const json& json,
        LazyResourceStore& store
    ) = 0;
    
    /** 
     * \brief Get the TypeId that this LazyResourceLoader implementation **MUST** load
     * \return The TypeId of the type that this loader loads
     */
    virtual TypeId id() = 0;

    friend class LazyResourceStore;
};

/**
 * \brief Class that all resource loaders should derive from providing a typed 
 * \tparam T The type that this loader will load
 */
template<typename T>
class LazyResourceLoader : private ErasedLazyResourceLoader {
public:
    /**
     * \brief Load a value of type `T` in a type-safe way
     * \param json The JSON value to load an element from
     * \param store A store containing other registered type loaders, if `T` has dependencies on other lazily loaded values
     * \return A reference to the loaded value
     * \throws Any exception that may occur when deserializing
     */
    virtual Ref<T> load(const json& json, LazyResourceStore& store) = 0;
    
    virtual ~LazyResourceLoader();
private:
    /** Override to safely implement the unsafe type-erasure functionality of `ErasedLazyResourceLoader` */
    Ref<void> load_untyped(const json& json, LazyResourceStore& store) override {
        return this->load(json, store);
    }
    /** Always returns the TypeId of `T` */
    TypeId id() override { return TypeId::id<T>(); }
};

/**
 * Exception thrown by a `LazyResourceStore` when a user attempts to deserialize a type that has no registered `LazyResourceLoader`
 */
struct UnregisteredResourceException : public std::exception {
public:
    const char * what() {
        return this->m_message.c_str();
    }

private:
    /** Cached error message */
    std::string m_message;
    
    UnregisteredResourceException(std::string&& msg) : m_message{msg} {}

    friend class LazyResourceStore;
};

/**
 * \brief Class containing values that can be lazily loaded by any registered `LazyResourceLoader` for the type, utilitizing
 * type erasure for runtime-registration of deserializers and better error messages
 */
class LazyResourceStore {
public:
    /** \brief Construct this resource store, performs no I/O operations */
    LazyResourceStore() : m_res{} {}
        
    /**
     * \brief Register a resource loader for type `T` that will be used for lazy loading
     * \param loader An instance of LazyResourceLoader
     */
    template<typename T>
    inline void register_loader(LazyResourceLoader<T>* loader) {
        this->register_loader<T>(std::unique_ptr{loader});
    }
    
    /**
     * \brief Register a resource loader for type `T` that will be used for lazy loading
     * \param loader An instance of LazyResourceLoader
     */
    template<typename T>
    void register_loader(std::unique_ptr<LazyResourceLoader<T>>&& loader) {
        this->m_res.emplace(
            loader->id().val(),
            loader
        );
    }
    
    /**
     * \brief Get a cached resource or load a new one from the given ID
     * \tparam T The type of lazily loaded resource to get
     * \param id ID to search for, used to do ID-to-folder lookup of the file to load
     * \return A shared reference to the loaded / cached resource
     * \throws Any exception that the lazy loader for `T` throws
     * \throws UnregisteredResourceException if `T` does not have a registered `LazyResourceLoader`
     */
    template<typename T>
    inline Ref<T> try_get() {
        auto id = TypeId::id<T>();
        return static_cast<Ref<T>>(this->try_get_id(id, typeid(T).name()));
    }

private:
    /**
     * \brief Where the magic happens, effectively maps `TypeId`s, which are just incremented counter values,
     * to their LazyResourceLoader values
     */
    std::unordered_map<std::size_t, std::unique_ptr<ErasedLazyResourceLoader>> m_res;
    
    /**
     * \brief Attempt to load a value using the registered lazy loader for the given type ID
     * \param id The ID of the type to load 
     * \param type_name Compile-time known typename of the type referenced by `id`
     * \return A type-erased reference to the value
     */
    Ref<void> try_get_id(TypeId id, const char *type_name);
};
