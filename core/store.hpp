#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <fstream>

#include "ser.hpp"
#include "util/hash.hpp"

/**
 * \brief Contains the name and file path to load a 
 * value from a JSON file, plus an optional weak reference to an
 * allocated component if it has been loaded
 * \sa GenericStore
 */
template<typename T>
struct GenericStoreEntry {
public:
    /** A pointer that is invalidated if the value is no longer used */
    std::weak_ptr<T> loaded;
    /** The name of the component type, shared with the Component instance */
    std::string name;
    /** Path to load the component from */
    std::filesystem::path path;
        
    /** Conver this cached entry to a JSON value */
    inline json to_json() const {
        return {
            {"name", this->name},
            {"path", this->path.c_str()}
        };
    }
    
    /** Convert this cache entry from a JSON value */
    static inline void from_json(GenericStoreEntry<T>& self, const json& j) {
        self.name = j["name"].get<std::string>();
        self.path = j["path"].get<std::filesystem::path>();
    }
    
    GenericStoreEntry() = default;
    inline GenericStoreEntry(const std::filesystem::path& p) : loaded{}, name{}, path{p} {};
    ~GenericStoreEntry() = default;
};

template<typename T, typename S>
class GenericStore { };

/**
 * \brief Concept specifying a type that provides methods to serialize and 
 * deserialize values of type T into a GenericStore
 * \sa GenericStore
 */
template<typename T, typename Contained>
concept GenericStoreSerializer = requires {
    typename T::IdType;
    {std::unordered_map<typename T::IdType, std::string>{}};
    {T::RESOURCE_DIR} -> std::convertible_to<const std::filesystem::path>;
    {T::load_id(std::declval<const json&>())} -> std::convertible_to<typename T::IdType>;
    {T::load_name(std::declval<const json&>())} -> std::convertible_to<std::string>;
    {T::load(
        std::declval<const json&>(), 
        std::declval<GenericStore<Contained, T>&>(),
        std::declval<const typename T::IdType&>(),
        std::declval<GenericStoreEntry<Contained>&>()
    )} -> std::same_as<std::shared_ptr<Contained>>;
    {T::save(std::declval<std::shared_ptr<Contained>>(), std::declval<GenericStore<Contained, T>&>())} -> std::convertible_to<json>;
};

namespace _detail {

/** Helper struct enabling an optimization when strings are used as the Id type of a GenericStore */
template<typename Id, typename T>
struct map_type_helper { using MapType = std::unordered_map<Id, GenericStoreEntry<T>>;};
/** Helper specialization enabling std::string_view's to be passed as IDs instead of string references */
template<typename T>
struct map_type_helper<std::string, T> { using MapType = std::unordered_map<std::string, GenericStoreEntry<T>, StringHasher, std::equal_to<>>; };

}

/**
 * \brief A generic resource manager that lazily loads values of type T.
 * Initialized by a cached value that maps IDs to names and file paths of lazily loaded
 * resources
 */
template<typename T, GenericStoreSerializer<T> Serializer>
class GenericStore<T, Serializer> {
public:
    using Ref = std::shared_ptr<T>;
    using WeakRef = std::weak_ptr<T>;
    using OptionalRef = std::optional<std::shared_ptr<T>>;
    using MapType = typename _detail::map_type_helper<typename Serializer::IdType, T>::MapType;    

    /** 
     * @brief Construct a new store, loading entries from a cache file.
     * If the cache file doesn't exist / is invalid then a new one will be generated (this may take some time)
     */
    GenericStore() {
        std::filesystem::path cachefile_path = Serializer::RESOURCE_DIR / "cache.json";
        try {
            std::ifstream cachefile{cachefile_path};
            json cache_json;
            cachefile >> cache_json;

            for(const auto& [id, entry_json] : cache_json.items()) {
                this->m_store.emplace(id, entry_json.template get<GenericStoreEntry<T>>());
            }
        } catch(const std::exception& e) {
            logger::warn("Failed to load a cached resource file {}, generating one", cachefile_path.c_str());
            this->generate_cachefile(cachefile_path);
        }
    }
   
    /** Move construct this generic store from another rvalue reference */
    GenericStore(GenericStore<T, Serializer>&& other) : m_store{std::move(other.m_store)} {}
    /** Move the assigned store into this one by assignment */
    inline GenericStore& operator=(GenericStore<T, Serializer>&& other) {
        this->m_store = std::move(other.m_store);
        return *this;
    }

    /** 
     * @brief Generate a cache file at the given path, also repopulating 
     * the internal ID to resource map using the new data 
     * @return true If the cache file was generated successfully
     */
    bool generate_cachefile(const std::filesystem::path& cachefile_path) {
        this->m_store.clear();
        auto resource_items = std::filesystem::recursive_directory_iterator{Serializer::RESOURCE_DIR};
        for(const auto& entry : resource_items) {
            if(entry.is_regular_file() && entry.path() != cachefile_path) {
                try {
                    std::ifstream entry_file{entry.path()};
                    json entry_json;
                    entry_file >> entry_json;
                    auto [elem, inserted] = this->m_store.emplace(Serializer::load_id(entry_json), entry.path());
                    if(!inserted) {
                        logger::warn(
                            "Two elements with ID {} detected while populating cache file, using {} over {}",
                            elem->first,
                            elem->second.path.c_str(),
                            entry.path().c_str()
                        );
                        continue;
                    }
                    elem->second.name = std::move(Serializer::load_name(entry_json));
                } catch(const std::exception& e) {
                    logger::error(
                        "Failed to read file {} while populating cache file {}: {}",
                        entry.path().c_str(),
                        cachefile_path.c_str(),
                        e.what()
                    );
                    continue;
                }
            } 
        }
        try {
            std::ofstream cachefile{cachefile_path};
            json::object_t cache_json{};
            for(const auto& [id, entry] : this->m_store) {
                cache_json.emplace(id, entry);
            }
            cachefile << cache_json;
        } catch(const std::exception& e) {
            logger::error("Failed to write cache file {}: {}", cachefile_path.c_str(), e.what());
            return false;
        }
        return true;
    }
    
    /**
     * @brief Attempt to retrieve a value by ID, and if it is not loaded return a default initialized reference
     * @return A reference to a default constructed value if the store *contains* an entry for the given ID but is has not
     * yet been loaded, or if the component has been loaded. Returns an empty reference if there exists no entry for the 
     * ID
     */
    template<typename Id>
    requires requires(const Id& id, MapType map) {
        map.find(id);  
    } 
    inline OptionalRef get_or_default(const typename Serializer::IdType& id) requires(std::is_default_constructible_v<T>) {
       auto stored = this->m_store.find(id);
        if(stored == this->m_store.end()) {
            return OptionalRef{};
        } else if(!stored->second.loaded.expired()) {
            return stored->second.loaded.lock();
        } else {
            Ref defaultRef = std::make_shared<T>();
            stored->second.loaded = WeakRef{defaultRef};
            return defaultRef;
        }
    }

    /**
     * @brief Attempt to get a value of type T by ID from this store, may cause a deserialization
     * of the value
     * @return A reference to the value stored / loaded, or an empty optional if the 
     * deserialization fails or no entry exists with the given name
     */
    template<typename Id>
    requires requires(const Id& id, MapType map) {
        map.find(id);
    }
    [[nodiscard("An unused shared reference will immediately be deallocated")]]
    OptionalRef get(const Id& id) {
        auto stored = this->m_store.find(id); 
        if(stored != this->m_store.end() && !stored->second.loaded.expired()) {
            return stored->second.loaded.lock();
        } else if(stored == this->m_store.end()) {
            logger::warn("Attempted to retrieve value by ID {} that does not exist", id);
            return OptionalRef{};
        }

        std::filesystem::path to_load = stored->second.path;
        try {
            std::ifstream json_file{to_load};
            json jsonval;
            json_file >> jsonval;
            Ref loaded = Serializer::load(
                jsonval,
                *this,
                stored->first,
                stored->second
            );
            stored->second.loaded = WeakRef{loaded};
            return OptionalRef{loaded};
        } catch(const std::exception& e) {
            logger::error("Failed to load value from {}: {}", to_load.c_str(), e.what());
            return OptionalRef{};
        }
    }
private:
    /** A map of all known IDs to store entries that may contain loaded values */
    MapType m_store;
};

