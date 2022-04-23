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
    /** Get this connection port's name */
    constexpr inline const std::string& name() const { return this->m_name; }
    /** Get this connection port's ID */
    constexpr inline const std::string& id() const { return this->m_id; }

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
    /** Internal ID of this connection port */
    std::string m_id;

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
        m_fp{std::move(other.m_fp)} {}
    Component() : m_name{}, m_id{}, m_ports{}, m_fp{} {};
    
    /** Get the name of this component */
    inline constexpr const std::string_view name() const { return this->m_name; }
    
    /** Get the connection port at the specified index */
    inline ConnectionPort const& get_port(const std::size_t idx) const { return this->m_ports[idx]; }
    /** Get a port by name, O(n) lookup time */
    std::optional<std::reference_wrapper<const ConnectionPort>> get_port(const std::string_view name) const;
    /** Get a port index by name */
    std::optional<std::size_t> get_port_idx(const std::string_view name) const;
private:
    /* @brief User-facing name of the component type, shared with the ComponentStore */
    std::string_view m_name;
    /* @brief ID string of this component, shared with the ComponentStore */
    std::string_view m_id;
    /* Connection points for this component, vector used instead of */
    std::vector<ConnectionPort> m_ports;
    /* @brief Shape of the component in the workspace */ 
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
 * @brief A weak reference to a component with additional internal port
 * number data
 */
struct PortRef {
public:
    /** Construct a port reference from a placed component reference and index of the port */
    inline PortRef(WeakComponentNodeRef ref, std::size_t idx) : m_ref{ref}, m_idx{idx} {}
    
    /** Serialize this port reference into an ID locator string */
    json to_json() const;

private:
    /** Reference to the placed component that this connection goes to */
    WeakComponentNodeRef m_ref;
    /** Index of the port in the m_conns vector */
    std::size_t m_idx;
};

/**
 * @brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 */
class ComponentNode {
public:
    ComponentNode() : m_ty{}, m_name{}, m_pos{}, m_conns{} {}
    
    /** Get the name of this component node */
    inline constexpr const std::string& name() const { return this->m_name; }
    inline constexpr std::size_t id() const { return this->m_id; }
    
    /** Fetch the underlying component type of this node */
    inline ComponentRef type() const { return this->m_ty; }
    
    ComponentNode(const std::size_t id) : m_ty{}, m_id{id}, m_name{}, m_pos{}, m_conns{} {}

private:
    /** What kind of component this is, shared with other components */
    ComponentRef m_ty;
    /** 
     * The internal ID of this component node, it is not a string because it
     * is never presented to the user and is stable between runs of the program
     */
    std::size_t m_id;
    /** User-assigned name of the placed part */
    std::string m_name;
    /** Offset in the workspace from center */
    Point m_pos;
    /** A vector sized the same as the component type's m_ports field */
    std::vector<std::optional<PortRef>> m_conns;
    

    friend class BoardGraph;
};

/**  
 * @brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
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
    using compmap_type = std::map<std::size_t, ComponentNodeRef>;
    /** A sparse array of internal IDs to placed components */
    compmap_type m_nodes;
    /** Collection of all loaded component types */
    ComponentStore m_store;
    /** ID counter for component IDs */
    std::size_t m_id_count = 0;
};


}
