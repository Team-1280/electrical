#pragma once

#include <filesystem>
#include <optional>

#include <ser.hpp>
#include <geom.hpp>


namespace model {

class ComponentId;

/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    /**
     * @brief Deserialize a component from a JSON value, throwing an
     * exception if the passed JSON is invalid
     */
    Component(const json& jsonval); 
        
    /** Convert this component to a JSON value */
    json to_json() const;
    
    Component(Component&& other) : 
        m_name{other.m_name},
        m_fp{std::move(other.m_fp)} {}

private:
    //! @brief User-facing name of the component type
    const char * const m_name;
    //! @brief Shape of the component in the workspace 
    Footprint m_fp;

    friend class BoardGraph;
};

static_assert(ser::JsonSerializable<Component>);

/**
 * @brief A unique identifier for a component that allows faster comparisons
 * due to string interning
 */
struct ComponentId {
public:
   constexpr bool operator==(const ComponentId& other) const {
        return this->m_str == other.m_str;
    } 
private:
    friend class ComponentStore;
    constexpr ComponentId(const std::string& str) : m_str{str.c_str()} {}
    const char * const m_str;
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
     * @brief Get a stored component type or load it from the cachefile
     */
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
        const std::string name;
        /** Path to load the component from */
        const std::string path;
        ~StoreEntry() = default;
    };

    using store_type = std::unordered_map<std::string, StoreEntry>;

    /** 
     * @brief ID to cache entry map used to speed up ID comparisons and allow lazy loading, eager freeing of memory 
     * Invariants:
     * StoreEntry's placed into m_store MUST not be removed, or else pointers stored in m_search
     * will become invalid and cause UB
     */
    store_type m_store;
};

}
