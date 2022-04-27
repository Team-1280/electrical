#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <fstream>
#include <tuple>

#include "ser.hpp"
#include "util/hash.hpp"

/**
 * \brief Contains the name and file path to load a 
 * value from a JSON file, plus an optional weak reference to an
 * allocated component if it has been loaded
 * \implements ser::ResourceManagerEntry
 * \sa ResourceManager
 */
template<typename T>
struct ResourceManagerEntry {
public:
    /** \brief A pointer that is invalidated if the value is no longer used */
    std::weak_ptr<T> loaded;
    /** \brief The name of the component type, shared with the Component instance */
    std::string name;
    /** \brief Path to load the component from */
    std::filesystem::path path;
        
    /** \brief Convert this cached entry to a JSON value */
    inline json to_json() const {
        return {
            {"name", this->name},
            {"path", this->path.c_str()}
        };
    }
    
    /** \brief Convert this cache entry from a JSON value */
    static inline void from_json(ResourceManagerEntry<T>& self, const json& j) {
        self.name = j["name"].get<std::string>();
        self.path = j["path"].get<std::filesystem::path>();
    }
    
    ResourceManagerEntry() = default;
    inline ResourceManagerEntry(const std::filesystem::path& p) : loaded{}, name{}, path{p} {};
    ~ResourceManagerEntry() = default;
};

/**
 * \brief Structure that must be specialized for a type T in order for
 * T to fulfill Generic
 */
template<typename T>
struct ResourceSerializer;
template<typename... Resources>
class GenericResourceManager;

namespace _detail {

/** \brief Helper struct enabling an optimization when strings are used as the Id type of a GenericStore */
template<typename Id, typename T>
struct map_type_helper { using MapType = std::unordered_map<Id, ResourceManagerEntry<T>>;};
/** \brief Helper specialization enabling std::string_view's to be passed as IDs instead of string references */
template<typename T>
struct map_type_helper<std::string, T> { using MapType = std::unordered_map<std::string, ResourceManagerEntry<T>, StringHasher, std::equal_to<>>; };

}

/**
 * \brief Concept that all template specializations of ResourceSerializer
 * must satisfy
 * \sa ResourceSerializer
 */
template<typename T, typename... Resources>
concept ResourceSerializerImpl = requires {
    typename T::IdType;
    {std::unordered_map<typename T::IdType, std::string>{}};
    {T::RESOURCE_DIR} -> std::convertible_to<const std::filesystem::path>;
    {T::load_id(std::declval<const json&>())} -> std::convertible_to<typename T::IdType>;
    {T::load_name(std::declval<const json&>())} -> std::convertible_to<std::string>;
    {T::load(
        std::declval<const json&>(), 
        std::declval<GenericResourceManager<Resources...>&>(),
        std::declval<const typename T::IdType&>(),
        std::declval<ResourceManagerEntry<T>&>()
    )} -> std::same_as<std::shared_ptr<T>>;
    {T::save(std::declval<std::shared_ptr<T>>(), std::declval<GenericResourceManager<Resources...>&>())} -> std::convertible_to<json>;
};

/**
 * \brief Concept specifying that a type T can be stored in a GenericResourceManager,
 * only requiring a Serializer specialization for T
 * \sa ResourceSerializer
 * \sa ResourceSerializerImpl
 */
template<typename T>
concept Resource = requires() {
    typename ResourceSerializer<T>;
    sizeof(ResourceSerializer<T>) != 0;
};

/**
 * \brief A generic resource manager that lazily loads values of type T.
 * Initialized by a cached value that maps IDs to names and file paths of lazily loaded
 * resources
 * \sa Resource
 * \sa ResourceSerializer
 * \sa ResourceSerializerImpl
 */
template<Resource... Values>
class GenericResourceManager<Values...> {
public:
    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T>
    using WeakRef = std::weak_ptr<T>;
    template<typename T>
    using OptionalRef = std::optional<std::shared_ptr<T>>;

    using SelfType = GenericResourceManager<Values...>;

    template<Resource T>
    using MapType = typename _detail::map_type_helper<typename ResourceSerializer<T>::IdType, T>::MapType;

    /*static_assert(
        (ResourceSerializerImpl<ResourceSerializer<Values>> && ...),
        "GenericResourceManager instantiated with value types containing no viable Serializer specialization"
    );*/

    /** 
     * \brief Construct a new store, loading entries from a cache file.
     * If the cache file doesn't exist / is invalid then a new one will be generated (this may take some time)
     */
    GenericResourceManager() {
        (construct_single_type<Values>(),...); 
    }
   
    /** \brief Move construct this generic store from another rvalue reference */
    GenericResourceManager(SelfType&& other) : m_stores{std::move(other.m_stores)} {}

    /** \brief  Move the assigned store into this one by assignment */
    inline SelfType& operator=(SelfType&& other) {
        this->m_stores = std::move(other.m_stores);
        return *this;
    }

    /** 
     * \brief Generate a cache file at the given path, also repopulating 
     * the internal ID to resource map using the new data 
     * \return true If the cache file was generated successfully
     */
    template<Resource T>
    requires requires {
        (std::is_same_v<T, Values> || ...);
        ResourceSerializerImpl<ResourceSerializer<T>, Values...>;
    }
    bool generate_cachefile(const std::filesystem::path& cachefile_path) {
        std::get<MapType<T>>(this->m_stores).clear();
        std::filesystem::recursive_directory_iterator resource_items;
        try {
            resource_items = std::filesystem::recursive_directory_iterator{ResourceSerializer<T>::RESOURCE_DIR};
        } catch(const std::exception& e) {
            logger::error(
                "Failed to get resource directory at {}: {}",
                ResourceSerializer<T>::RESOURCE_DIR.c_str(),
                e.what()
            );
            return false;
        }
        for(const auto& entry : resource_items) {
            if(entry.is_regular_file() && entry.path() != cachefile_path) {
                try {
                    std::ifstream entry_file{entry.path()};
                    json entry_json;
                    entry_file >> entry_json;
                    auto [elem, inserted] = std::get<MapType<T>>(this->m_stores).emplace(ResourceSerializer<T>::load_id(entry_json), entry.path());
                    if(!inserted) {
                        logger::warn(
                            "Two elements with ID {} detected while populating cache file, using {} over {}",
                            elem->first,
                            elem->second.path.c_str(),
                            entry.path().c_str()
                        );
                        continue;
                    }
                    elem->second.name = std::move(ResourceSerializer<T>::load_name(entry_json));
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
            for(const auto& [id, entry] : std::get<MapType<T>>(this->m_stores)) {
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
     * \brief Attempt to retrieve a value by ID, and if it is not loaded return a default initialized reference
     * \return A reference to a default constructed value if the store *contains* an entry for the given ID but is has not
     * yet been loaded, or if the component has been loaded. Returns an empty reference if there exists no entry for the 
     * ID
     */
    template<Resource T, typename Id>
    requires requires(const Id& id, MapType<T> map) {
        map.find(id);
        std::is_default_constructible_v<T>;
        ResourceSerializerImpl<ResourceSerializer<T>, Values...>;
    } 
    inline OptionalRef<T> get_or_default(const typename ResourceSerializer<T>::IdType& id) {
       auto stored = std::get<MapType<T>>(this->m_stores).find(id);
        if(stored == std::get<MapType<T>>(this->m_stores).end()) {
            return OptionalRef<T>{};
        } else if(!stored->second.loaded.expired()) {
            return stored->second.loaded.lock();
        } else {
            Ref<T> defaultRef = std::make_shared<T>();
            stored->second.loaded = WeakRef<T>{defaultRef};
            return defaultRef;
        }
    }

    /**
     * \brief Attempt to get a value of type T by ID from this store, may cause a deserialization
     * of the value
     * \return A reference to the value stored / loaded, or an empty optional if the 
     * deserialization fails or no entry exists with the given name
     */
    template<Resource T, typename Id>
    requires requires(const Id& id, MapType<T> map) {
        map.find(id);
        ResourceSerializerImpl<ResourceSerializer<T>, Values...>;
    }
    [[nodiscard("An unused shared reference will immediately be deallocated")]]
    OptionalRef<T> get(const Id& id) {
        auto stored = std::get<MapType<T>>(this->m_stores).find(id); 
        if(stored != std::get<MapType<T>>(this->m_stores).end() && !stored->second.loaded.expired()) {
            return stored->second.loaded.lock();
        } else if(stored == std::get<MapType<T>>(this->m_stores).end()) {
            logger::warn("Attempted to retrieve value by ID {} that does not exist", id);
            return OptionalRef<T>{};
        }

        std::filesystem::path to_load = stored->second.path;
        try {
            std::ifstream json_file{to_load};
            json jsonval;
            json_file >> jsonval;
            Ref loaded = ResourceSerializer<T>::load(
                jsonval,
                *this,
                stored->first,
                stored->second
            );
            stored->second.loaded = WeakRef{loaded};
            return OptionalRef{loaded};
        } catch(std::exception& e) {
            logger::error("Failed to load value from {}: {}", to_load.c_str(), e.what());
            return OptionalRef<T>{};
        }
    }
private:
    template<Resource T>
    inline void construct_single_type() {
        std::filesystem::path cachefile_path = ResourceSerializer<T>::RESOURCE_DIR / "cache.json";
        try {
            std::ifstream cachefile{cachefile_path};
            json cache_json;
            cachefile >> cache_json;

            for(const auto& [id, entry_json] : cache_json.items()) {
                std::get<MapType<T>>(this->m_stores).emplace(id, entry_json.template get<ResourceManagerEntry<T>>());
            }
        } catch(std::exception& e) {
            logger::warn("Failed to load a cached resource file {}, generating one", cachefile_path.c_str());
            this->generate_cachefile<T>(cachefile_path);
        }
    }

    /** \brief A map of all known IDs to store entries that may contain loaded values */
    std::tuple<MapType<Values>...> m_stores;
};

