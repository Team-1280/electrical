#pragma once

#include <filesystem>
#include <fstream>
#include <optional>

#include <ser.hpp>
#include <geom.hpp>
#include "util/hash.hpp"

namespace model {

/**
 * @brief A structure defining a single connection point on a component
 */
struct ConnectionPort {
public:
    /** Get this connection port's name */
    constexpr inline const std::string& name() const { return this->m_name; }
    /** Get this connection port's ID */
    constexpr inline const std::string_view id() const { return this->m_id; }

    inline ConnectionPort(ConnectionPort&& other) : m_pt{std::move(other.m_pt)}, m_name{std::move(other.m_name)}, m_id{std::move(other.m_id)} {}
    inline ConnectionPort& operator=(ConnectionPort&& other) {
        this->m_pt = other.m_pt;
        this->m_name = std::move(other.m_name);
        this->m_id = std::move(other.m_id);
        return *this;
    }

    static void from_json(ConnectionPort& self, const json& j);
    json to_json() const;
    ConnectionPort() : m_pt{}, m_name{}, m_id{} {}
private:
    /** Location on the component's footprint that this connection port is located */
    Point m_pt;
    /** Name of the port */
    std::string m_name;
    /** Internal ID of this connection port, shared with the parent Component and guranteed to be NULL terminated */
    std::string_view m_id;

    friend class ComponentStore;
    friend class Component;
};

static_assert(ser::JsonSerializable<ConnectionPort>);

/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    /** Convert this component to a JSON value */
    json to_json() const;
    
    Component(Component&& other) : 
        m_name{other.m_name},
        m_id{other.m_id},
        m_ports{std::move(other.m_ports)},
        m_fp{std::move(other.m_fp)} {}
    Component() : m_name{}, m_id{}, m_ports{}, m_fp{} {};
    
    /** Get the name of this component */
    inline constexpr const std::string_view name() const { return this->m_name; }
    
    /** Get a port by name, O(n) lookup time */
    std::optional<std::reference_wrapper<const ConnectionPort>> get_port(const std::string& id) const;
    /** Get a port index by name */
    std::optional<std::size_t> get_port_idx(const std::string_view name) const;
private:
    /* @brief User-facing name of the component type, shared with the ComponentStore */
    std::string_view m_name;
    /* @brief ID string of this component, shared with the ComponentStore */
    std::string_view m_id;
    /* 
     * Map of IDs to connection points for this component 
     * Note: The WireEdge class contains pointers into this map, meaning that
     * after construction elements MUST not be removed
     */
    std::unordered_map<std::string, ConnectionPort, StringHasher> m_ports;
    /* @brief Shape of the component in the workspace */ 
    Footprint m_fp;
    
    friend class ComponentStore;
    friend class BoardGraph;
};

using ComponentRef = std::shared_ptr<Component>;

template<typename T, typename Serializer>
class GenericStore {public: struct StoreEntry; };

/**
 * @brief Concept specifying a type that provides methods to serialize and 
 * deserialize values of type T into a GenericStore
 */
template<typename T, typename Contained>
concept GenericStoreSerializer = requires {
    //{T::RESOURCE_DIR} -> std::same_as<std::filesystem::path>;
    {T::load_id(std::declval<const json&>())} -> std::convertible_to<std::string>;
    {T::load_name(std::declval<const json&>())} -> std::convertible_to<std::string>;
    /*{T::load(
        std::declval<const json&>(), 
        std::declval<GenericStore<T, Contained>&>(),
        std::declval<const std::string&>(),
        std::declval<typename GenericStore<T, Contained>::StoreEntry&>()
    )} -> std::same_as<std::shared_ptr<T>>;
    {T::save(std::declval<std::shared_ptr<T>>(), std::declval<GenericStore<T, Contained>&>())} -> std::convertible_to<json>;*/
};

/**
 * @brief A generic resource manager that lazily loads values of type T.
 * Initialized by a cached value that maps IDs to names and file paths of lazily loaded
 * resources
 */
template<typename T, GenericStoreSerializer<T> Serializer>
class GenericStore<T, Serializer> {
public:
    using Ref = std::shared_ptr<T>;
    using WeakRef = std::weak_ptr<T>;
    using OptionalRef = std::optional<std::shared_ptr<T>>;
    
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
                this->m_store.emplace(id, entry_json.get<StoreEntry>());
            }
        } catch(const std::exception& e) {
            logger::warn("Failed to load a cached resource file {}, generating one", cachefile_path.c_str());
            this->generate_cachefile(cachefile_path);
        }
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
                            elem->second.path,
                            entry.path().c_str()
                        );
                        continue;
                    }
                    elem->name = std::move(Serializer::load_name(entry_json));
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
    
    /** Get a value of type T by ID from this store */
    inline OptionalRef get(const std::string& id) { return this->get(std::string_view{id}); }
    /**
     * @brief Attempt to get a value of type T by ID from this store, may cause a deserialization
     * of the value
     * @return A reference to the value stored / loaded, or an empty optional if the 
     * deserialization fails or no entry exists with the given name
     */
    [[nodiscard("An unused shared reference will immediately be deallocated")]] OptionalRef get(const std::string_view id) {
        auto stored = this->m_store.find(id); 
        if(stored != this->m_store.end() && !stored->second->loaded.expired()) {
            return stored->second->loaded.get();
        } else if(stored == this->m_store.end()) {
            logger::warn("Attempted to retrieve value by ID {} that does not exist", id);
            return OptionalRef{};
        }

        std::filesystem::path to_load = stored->second->path;
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
            stored->second->loaded = WeakRef{loaded};
            return OptionalRef{loaded};
        } catch(const std::exception& e) {
            logger::error("Failed to load value from {}: {}", to_load.c_str(), e.what());
            return OptionalRef{};
        }
    }
private:
    /**
     * @brief Contains the name and file path to load a 
     * value from a JSON file, plus an optional weak reference to an
     * allocated component if it has been loaded
     */
    struct StoreEntry {
    public:
        /** A pointer that is invalidated if the value is no longer used */
        WeakRef loaded;
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
        inline void from_json(StoreEntry& self, const json& j) {
            self.name = j["name"].get<std::string>();
            self.path = j["path"].get<std::filesystem::path>();
        }
       
        StoreEntry() = default;
        inline StoreEntry(const std::filesystem::path& p) : loaded{}, name{}, path{p} {};
        ~StoreEntry() = default;
    };
    static_assert(ser::JsonSerializable<StoreEntry>);

    using MapType = std::unordered_map<std::string, StoreEntry, StringHasher>;
    /** A map of all known IDs to store entries that may contain loaded values */
    MapType m_store;
};

/**
 * Class with static methods to serialize components to and from JSON
 * values in a GenericStore
 */
struct ComponentSerializer {
public:
    static const std::filesystem::path RESOURCE_DIR;
    using Store = GenericStore<Component, ComponentSerializer>;

    static std::string load_id(const json&);
    static std::string load_name(const json&);
    static std::shared_ptr<Component> load(
        const json&,
        GenericStore<Component, ComponentSerializer>&,
        const std::string&,
        Store::StoreEntry&
    );
    static json save(std::shared_ptr<Component>, Store&);
};

static_assert(GenericStoreSerializer<ComponentSerializer, Component>);

/** 
 * @brief A structure storing component types with methods to lazy load
 */
class ComponentStore {
public:
    /** @brief Create a new component store */
    ComponentStore();
    
    /**
     * @brief Load this component storage from a JSON cache file
     */
    static void from_json(ComponentStore& self, const json& j);
    
    /** @brief Convert this component store into a cache file */
    json to_json() const;
    
    /**
     * @brief Get a stored component type or load it from the cachefile
     */
    [[nodiscard("If a component reference is not stored, it will be deallocated immediately")]] 
    std::optional<ComponentRef> find(const std::string& id);

private:    
    static const std::filesystem::path COMPONENT_DIR;
    static const std::filesystem::path CACHEFILE_PATH;
    

    /**
     * @brief Contains the name and file path to load a 
     * component from a JSON file, plus an optional weak reference to an
     * allocated component if it has been loaded
     */
    struct StoreEntry {
        /** A pointer that is invalidated if the component type is no longer used */
        std::weak_ptr<Component> loaded;
        /** The name of the component type, shared with the Component instance */
        std::string name;
        /** Path to load the component from */
        std::string path;
        
        StoreEntry() = default;
        static void from_json(StoreEntry& self, const json& j);
        json to_json() const;
        ~StoreEntry() = default;
    };
    static_assert(ser::JsonSerializable<StoreEntry>);

    using store_type = std::unordered_map<std::string, StoreEntry, StringHasher>;

    /** 
     * @brief ID to cache entry map used to speed up ID comparisons and allow lazy loading, eager freeing of memory 
     * Invariants:
     * StoreEntry's placed into m_store MUST not be removed, or else pointers stored in m_search
     * will become invalid and cause UB
     */
    store_type m_store;
    
    /** Load a component from a JSON file at the given path, returning a shared pointer to it */
    ComponentRef load(const std::string& path);
};

static_assert(ser::JsonSerializable<ComponentStore>);

}
