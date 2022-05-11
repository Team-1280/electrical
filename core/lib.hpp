#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <uuid.h>

#include "geom.hpp"
#include "component.hpp"
#include "wire.hpp"
#include "ser.hpp"
#include "resource.hpp"
#include "unit.hpp"


/**
 * Span that references a uuids::uuid 
 */
using uuidref = uuids::span<std::byte const, 16L>;


class ComponentNode;

/**
 * \brief An edge in the board graph representing a single wire connection between two
 * ports on a component
 *
 * \sa Connector
 * 
 * \implements Resource
 */
class WireEdge {
public:
    /** 
     * \brief Enumeration representing the 'sides' of a wire, the names don't mean 
     * anything but ensure that different wire ends can be represented in code
     */
    enum Side: std::uint8_t {
        LEFT = 0,
        RIGHT = 1
    };

    /** 
     * A \brief single end of a wire, that may be free-floating (not connected to any component node)
     * or connected to a specific port on a component node
     */
    struct Connection {
    public:
        /** Create a new connection point using a component and pointer to a connection port */
        inline Connection(WeakRef<ComponentNode> component, ConnectionPort * port) :
            m_component{component}, m_port{port} {}
    
        /**
         * \brief Get the port that this connection is attached to on the component node
         *
         * \return an empty std::optional if this connection end is not attached to any node in the graph
         * or a reference to the connection port on the attached component this is attached to
         */
        std::optional<std::reference_wrapper<const ConnectionPort>> port() const;
        /** \brief Get the graph node that this connection is attached to */ 
        inline WeakRef<ComponentNode> component() const noexcept { return this->m_component; }
        /** \brief Get the connector type of this connection point */
        inline Ref<Connector> connector() const noexcept { return this->m_connector; }
        /**
         * \brief Check if this connection is attached to a graph node
         * \return true if this connection point does not attach to a node in the graph
         */
        inline constexpr bool is_floating() { return this->m_component.expired(); }
                
        /**
         * \brief Detach this wire end from the component node's port, if
         * it is connected at all
         */
        void detach();

    private:
        /** \brief Node in the graph that this edge connects to */
        WeakRef<ComponentNode> m_component;
        
        /**
         * \brief Union containing either the port that this wire end connects to on 
         * m_component or the position of the wire end in workspace coordinates if the 
         * end is 'floating'
         */
        union {
            /** 
             * \brief Pointer to a connection port on the component node's type 
             *
             * Implementation note: This MUST not be invalid if m_component is not expired
             */
            ConnectionPortRef m_port;
            
            /** \brief Position of the connector in the workspace if this end is 'floating' */
            Point m_pos;
        };

        /** \brief A shared resource pointing to a user-defined connector on this connection point */
        Ref<Connector> m_connector;

        Connection() : m_component{}, m_port{} {}
        friend class WireEdge;
        friend class BoardGraph;
    };
    
    /** \brief Get the UUID of this wire edge */
    constexpr inline uuidref id() const { return this->m_id; }
    /** \brief Get the pair of Connection structures representing the ends of this wire edge */
    inline const std::array<Connection, 2>& connections() const { return this->m_conns; }
    /** 
     * \brief Check if this edge connects to the given node in the graph
     * \return true if this edge attaches to the given node
     */
    inline bool connects(const Ref<ComponentNode>& node) const { 
        return std::any_of(
            this->m_conns.begin(),
            this->m_conns.end(),
            [&node](const auto& conn) { return (conn.m_component.expired()) ? false : conn.m_component.lock() == node; } 
        );
    }
    
    /**
     * \brief Convenience method to fetch a wire end by side 
     */
    inline constexpr Connection& side(const Side side) { return this->m_conns[side]; }
    
private:
    /** \brief Components that this wire connects between*/
    std::array<Connection, 2> m_conns;
    /** \brief Internal ID number of this wire edge */
    uuidref m_id;
    /** \brief User-placed points that this wire travels between on the workspace */
    std::vector<Point> m_wire_pts;

    WireEdge() : m_conns{}, m_id{nullptr, 16} {};

    friend class BoardGraph;
};

/**
 * \brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 * 
 * \sa WireEdge
 * \implements Resource
 */
class ComponentNode {
public:
    /**
    * \brief Structure containing a reference to an edge in the graph
    * and the side of the wire that connects to this node
    */
    struct EdgeConnection {
       /** \brief A reference to the wire that connects to this node */
       Ref<WireEdge> edge;
       /** \brief What side of the wire connects to this component */
       WireEdge::Side side;
    };

    ComponentNode() : m_ty{}, m_id{nullptr, 16}, m_name{}, m_pos{} {}
    
    /** \brief Get the name of this component node */
    inline constexpr const std::string& name() const { return this->m_name; }
    inline constexpr uuidref id() const { return this->m_id; }
    
    /** \brief Fetch the underlying component type of this node */
    inline Ref<Component> type() const noexcept { return this->m_ty; }
    ComponentNode(const uuids::uuid& id) : m_ty{}, m_id{id.as_bytes()}, m_name{}, m_pos{} {}
        
    /**
     * \brief Get the wires connected on to a specific port on this component
     * \param Port a reference to the port to query for connections
     * \return A structure detailing all connected edges on the specific port, or 
     * an empty optional if there is no connection to the port
     */
    Optional<std::reference_wrapper<EdgeConnection>> port(ConnectionPortRef port);
    
    /**
     * \brief Add a port if it does not exist by port reference
     * \param port a reference to the connection port on this component's type
     * \param edge The wire edge to connect to the given port
     * \param side The side of the wire edge to connect
     * \param force If any existing connection on the given port should be removed
     * \return A reference to the added or existing connection structure or 
     * an empty optional if this component's type does not have the given port or 
     * force is false and there is already a connection to the port
     */
    Optional<std::reference_wrapper<ComponentNode::EdgeConnection>> connnect_port(ConnectionPortRef port, Ref<WireEdge> edge, const WireEdge::Side side, bool force = true);
    
    /**
     * \brief Remove a port's connections from this node
     * \param port The port to remove connections from
     */
    inline void remove_port(ConnectionPortRef port) {
        this->m_edges.erase(port);
    }
    
    /** \brief Get the position of this component */
    inline constexpr const Point& pos() const { return this->m_pos; }
private:
    /** \brief What kind of component this is, shared with other components */
    Ref<Component> m_ty;
    /** 
     * \brief The internal ID of this component node, it is not a string because it
     * is never presented to the user and is stable between runs of the program
     */
    uuidref m_id;
    /** \brief User-assigned name of the placed part */
    std::string m_name;
    /** \brief Offset in the workspace from center */
    Point m_pos;
    
    /** \brief All graph edges connecting this component node to others */
    std::map<ConnectionPortRef, EdgeConnection> m_edges;

    friend class BoardGraph;
    friend class ConnectedNodesIterator;
};

}

template<>
struct ResourceSerializer<model::ComponentNode> {
    using ComponentNode = model::ComponentNode;
    using IdType = uuids::uuid;
    static const std::filesystem::path RESOURCE_DIR;
    using Preloaded = SinglePreload<std::string, 'n', 'a', 'm', 'e'>;

    static inline IdType save_id(const IdType& id) { return id; }
    static inline IdType load_id(const json& json_val) { return json_val.get<uuids::uuid>(); }
    static inline json save(ComponentNode& component) {
        json::object_t obj{};
        obj.emplace("name", component.m_name);
        obj.emplace("id", component.m_id);
        obj.emplace("type", component.m_ty->id());
        obj.emplace("conns", json::array({}));
        for(const auto& [port, conn] : component.m_edges) {
            obj["conns"].push_back({
                {"port", port->id()},
                {"edge", conn.edge->id()},
                {"side", conn.side}
            });
        }

        return obj;
    }
};
template<>
struct ResourceSerializer<model::WireEdge> {
    using WireEdge = model::WireEdge;
    using IdType = uuids::uuid;
    static const std::filesystem::path RESOURCE_DIR;

    using Preloaded = NoPreload;
    static inline IdType save_id(const IdType& id) { return id; }
    static inline IdType load_id(const json& json_val) { return json_val.get<uuids::uuid>(); }
    template<Resource... Resources>
    static inline json save(WireEdge& wire) {
        json::object_t obj{};
        obj.emplace("id", wire.m_id);
        obj.emplace("conns", json::array({}));
        for(const auto& conn : wire.m_conns) {
            if(conn.m_component.expired()) {
                obj.at("conns").push_back(nullptr);
            }
            auto component = conn.m_component.lock();
            json::object_t conn_obj{};
            conn_obj.emplace("node", component->id());
            conn_obj.emplace("port", conn.m_port->id());
            conn_obj.emplace("connector", conn.m_connector->id());
            obj.at("conns").push_back(conn_obj);
        }
    
        return obj;
    }

    template<Resource... Resources>
    requires(
        HasResource<model::ComponentNode, Resources...> &&
        HasResource<model::Connector, Resources...>
    )
    static inline void load(
        Ref<WireEdge> wire,
        const json& json_val,
        GenericResourceManager<Resources...> res,
        const IdType& id,
        NoPreload&
    ) {
        wire->m_id = id.as_bytes();
        for(std::size_t i = 0; const auto& conn_json : json_val.at("conns")) {
            if(conn_json.is_null() || i >= 2) {
                continue;
            }
            
            wire->m_conns[i].m_component = res.template try_get<model::ComponentNode>(conn_json.at("node").get<uuids::uuid>());
            wire->m_conns[i].m_port = *wire->m_conns[i].m_component.lock()->type()->get_port_ref(conn_json.at("port").get<std::string_view>());
            wire->m_conns[i].m_connector = res.template try_get<model::Connector>(conn_json.at("connector").get<std::string_view>());
        }
    }
};

/**  
 * \brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 *
 * \implements ser::FromJson
 */
class BoardGraph {
public:
    /**
     * \brief Initialize this board graph, loading or regenerating
     * cached resource files 
     */
    BoardGraph() : m_res{} {}
    
    /**
     * \brief Load a board graph from a saved file, or create a new save file with the given file
     */
    BoardGraph(std::filesystem::path&&);

    inline BoardGraph(BoardGraph&& other) : m_res{std::move(other.m_res)} {}
    inline BoardGraph& operator=(BoardGraph&& other) {
        this->m_res = std::move(other.m_res);
        return *this;
    }
    
    /**
     * \brief Create a new component node with the given type 
     * \param type The type of component to create
     * \return A reference to the created graph node
     */
    Ref<ComponentNode> component(Ref<Component> type, std::string_view name); 
    
    /**
     * \brief Load a board graph from a JSON file
     */
    static void from_json(BoardGraph&, const json&); 
    /** \brief Save this board graph to a file */
    json to_json() const;

private:
    Optional<Ref<ComponentNode>> load_node(const uuids::uuid id, const json& json_val);
    Optional<Ref<ComponentNode>> load_edge(const uuids::uuid id, const json& json_val);

    /** \brief Collection of all loaded component types */
    SharedResources m_res;

    Map<uuids::uuid, Ref<ComponentNode>> m_nodes;
    Map<uuids::uuid, Ref<WireEdge>> m_edges;
    
    /** \brief Path to a file used for saving and loading this board graph */
    std::filesystem::path m_path; 
};

