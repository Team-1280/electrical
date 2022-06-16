#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>

#include "geom.hpp"
#include "component.hpp"
#include "ser/store.hpp"
#include "wire.hpp"
#include "ser/ser.hpp"
#include "unit.hpp"


class ComponentNode;

/**
 * \brief An edge in the board graph representing a single wire connection between two
 * ports on a component
 *
 * \sa Connector
 * 
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
     * \brief A single end of a wire, that may be free-floating (not connected to any component node)
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
    
        /**
         * \brief Get the position of this connector, fetched either from the port that this is connected to
         * or the stored position
         * \return Position that this end occupies
         */
        Point const& pos() const;
        /** \brief Get the graph node that this connection is attached to */ 
        inline WeakRef<ComponentNode> component() const noexcept { return this->m_component; }
        /** \brief Get the connector type of this connection point */
        inline Ref<Connector> connector() const noexcept { return this->m_connector; }
        /**
         * \brief Check if this connection is attached to a graph node
         * \return true if this connection point does not attach to a node in the graph
         */
        inline bool is_floating() const { return this->m_component.expired(); }
                
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
    
    /** \brief Get the ID of this wire edge */
    constexpr inline const std::string_view id() const noexcept { return this->m_id; }
    /** \brief Get the pair of Connection structures representing the ends of this wire edge */
    inline std::array<Connection, 2> const& connections() const noexcept { return this->m_conns; }
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

    std::vector<Point>::const_iterator begin() const { return this->m_wire_pts.begin(); }
    std::vector<Point>::const_iterator end() const { return this->m_wire_pts.end(); }
private:
    /** \brief Components that this wire connects between*/
    std::array<Connection, 2> m_conns;
    /** \brief Internal ID string of this wire edge */
    std::string_view m_id;
    /** \brief User-placed points that this wire travels between on the workspace */
    std::vector<Point> m_wire_pts;

    WireEdge() : m_conns{}, m_id{}, m_wire_pts{} {};

    friend class BoardGraph;
};

/**
 * \brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 * 
 * \sa WireEdge
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

    ComponentNode() : m_ty{}, m_id{}, m_name{}, m_pos{} {}
    
    /** \brief Get the name of this component node */
    inline constexpr const std::string& name() const { return this->m_name; }
    inline constexpr const std::string_view id() const { return this->m_id; }
    
    /** \brief Fetch the underlying component type of this node */
    inline Ref<Component> type() const noexcept { return this->m_ty; }
    ComponentNode(const std::string_view id) : m_ty{}, m_id{id}, m_name{}, m_pos{} {}
        
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
     * \brief The user-assigned ID of this component node
     */
    std::string_view m_id;
    /** \brief User-assigned name of the placed part */
    std::string m_name;
    /** \brief Offset in the workspace from center */
    Point m_pos;
    
    /** \brief All graph edges connecting this component node to others */
    std::map<ConnectionPortRef, EdgeConnection> m_edges;

    friend class BoardGraph;
    friend class ConnectedNodesIterator;
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
    BoardGraph() = default;
    
    /**
     * \brief Load a board graph from a saved JSON file, or create a new save file with the given file
     * \param path Path to a JSON save file that the board graph is stored in
     * \param create If the file at the given path does not exist, should we create it?
     * \param save Wether this board graph should write itself to the save file when the destructor runs
     * \throws std::runtime_error if `create` is false and the file does not exist
     * \throws std::runtime_error if the file at the given path exists but could not be parsed
     */
    BoardGraph(std::filesystem::path&& path, bool create = false, bool save = true);

    inline BoardGraph(BoardGraph&& other) = default; 
    inline BoardGraph& operator=(BoardGraph&& other) = default;
    
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
    
    /**
     * \brief Get or load a node in this graph by ID
     * \param id UUID of the loaded node
     * \return An empty optional if the file for the UUID does not exist
     */
    Optional<Ref<ComponentNode>> get_node(const std::string_view id) const;
    /**
     * \brief Get or load an edge in this graph by ID
     * \param id UUID of the loaded edge
     * \return An empty optional if the file does not exis
     */
    Optional<Ref<WireEdge>> get_edge(const std::string_view id) const;
    
    /** Save this graph to a file */
    virtual ~BoardGraph();
    
    /**
     * \brief structure with `begin` and `end` methods to allow an iteration over nodes in a board graph
     * using an enhanced for loop
     */
    struct NodeIterator {
    public:
        using iterator_type = Map<std::string, Ref<ComponentNode>>::iterator;

        constexpr NodeIterator(BoardGraph& graph) : m_graph{graph} {}
        inline iterator_type begin() { return this->m_graph.m_nodes.begin(); }
        inline iterator_type end() { return this->m_graph.m_nodes.end(); }
    private:
        BoardGraph& m_graph;
    };
    
    /**
     * \brief Structure referencing a `BoardGraph` that allows iteration over edges in the graph
     * using an enhanced for loop
     */
    struct EdgeIterator {
    public:
        using iterator_type = Map<std::string, Ref<WireEdge>>::iterator;
        constexpr EdgeIterator(BoardGraph& graph) : m_graph{graph} {}
        inline iterator_type begin() { return this->m_graph.m_edges.begin(); }
        inline iterator_type end() { return this->m_graph.m_edges.end(); }
    private:
        BoardGraph& m_graph;
    };
    
    /** \brief Get an iterator over all nodes stored in this graph */
    inline constexpr NodeIterator nodes() noexcept { return NodeIterator{*this}; }
    
    /** \brief Get an iterator over all edges stored in the graph */
    inline constexpr EdgeIterator edges() noexcept { return EdgeIterator{*this}; }

private:
    /** \brief Collection of all loaded component types */
    LazyResourceStore m_res;
    
    /** \brief Map of internal node IDs to shared node references */
    Map<std::string, Ref<ComponentNode>> m_nodes;
    /** \brief Map of internal node IDs to shared node references */
    Map<std::string, Ref<WireEdge>> m_edges;
    
    /**
     * \brief Ensure that a node with the given ID has been loaded from the root object
     * \param id The ID string of the node, must be a valid UUID or the node will not be loaded
     * \param obj Root JSON object passed to `from_json`
     */
    void load_node(const std::string& id, const json::object_t& obj);
    /**
     * \brief Ensure that an edge with the given ID has been loaded from the root object
     * \param id The ID string, must be a valid UUID
     * \param obj Root JSON object passed to `from_json`
     */
    void load_edge(const std::string& id, const json::object_t& obj);
    
    /** \brief Path to a file used for saving and loading this board graph */
    std::filesystem::path m_path;
    
    /** \brief If we should serialize this board to our stored save file on destruction */
    bool m_save{false};
};
