#pragma once

#include <filesystem>
#include <optional>

#include <ser.hpp>
#include <geom.hpp>


namespace model {

/**
 * @brief A structure defining a single connection point on a component
 */
struct ConnectionPort {
public:
    /** Get a string view into this connection port's name */
    constexpr inline const std::string_view name() const { return this->m_name; }
    /** Get a C-style NULL-terminated name string */
    constexpr inline const char * const c_str() const { return this->m_name.data(); }
private:
    /** Location on the component's footprint that this connection port is located */
    Point m_pt;
    /** 
     * Name of the port, shared with the parent Component
     * Guranteed to be NULL-terminated
     */
    std::string_view m_name;

    friend class ComponentStore;
    friend class Component;
    ConnectionPort() : m_pt{}, m_name{} {}
};


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
        m_fp{std::move(other.m_fp)} {}


    Component() : m_name{}, m_id{}, m_conns{}, m_fp{} {};
private:
    //! @brief User-facing name of the component type, shared with the ComponentStore
    std::string_view m_name;
    //! @brief ID string of this component, shared with the ComponentStore
    std::string_view m_id;
    //! @brief Connection points for this component
    std::unordered_map<std::string, ConnectionPort> m_conns;
    //! @brief Shape of the component in the workspace 
    Footprint m_fp;
    
    friend class ComponentStore;
    friend class BoardGraph;
};


using ComponentRef = std::shared_ptr<Component>;

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

    using store_type = std::unordered_map<std::string, StoreEntry>;

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
