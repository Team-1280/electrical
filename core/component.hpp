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

class ComponentNode;

/**
 * @brief A shared reference to a ComponentNode, this reference must
 * NOT outlive the BoardGraph that creates it as the ComponentNode shares data with 
 * the BoardGraph that will become invalidated when the BoardGraph is destructed
 */
using ComponentNodeRef = std::shared_ptr<ComponentNode>;
using WeakComponentNodeRef = std::weak_ptr<ComponentNode>;

/**
 * @brief A hash map utilizing perfect hashing to map a constant set
 * of strings to values
 */
struct PortMap {
public:

private:
    std::vector<WeakComponentNodeRef> m_slots;
    std::uint64_t m_multiplier;
    
    PortMap() : m_slots{}, m_multiplier{} {}
};

/**
 * @brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 */
class ComponentNode {
public:
    ComponentNode() : m_ty{}, m_name{}, m_pos{} {}

private:
    /** What kind of component this is, shared with other components */
    ComponentRef m_ty;
    /** User-assigned name of the placed part, shared with the board graph */
    std::string_view m_name;
    /** Offset in the workspace from center */
    Point m_pos;

    friend class BoardGraph;
};


/**  
 * @brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
    /** Get a component node by name from this graph */
    std::optional<ComponentNodeRef> get_node(const std::string& s);
    
    /** Deserialize a board graph from a JSON value */
    static void from_json(BoardGraph& self, const json& j);
    /** Serialize this board graph to a JSON value */
    json to_json() const;

    BoardGraph() : m_nodes{}, m_store{} {}    
    inline BoardGraph(BoardGraph&& other) : m_nodes{std::move(other.m_nodes)}, m_store{std::move(other.m_store)} {}
    inline BoardGraph& operator=(BoardGraph&& other) {
        this->m_nodes = std::move(other.m_nodes);
        this->m_store = std::move(other.m_store);
        return *this;
    }
private:
    using compmap_type = std::unordered_map<std::string, ComponentNodeRef>;
    /** A map of user-assigned node names to placed components */
    compmap_type m_nodes;
    /** Collection of all loaded component types */
    ComponentStore m_store;
};


}
