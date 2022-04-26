#pragma once

#include <array>

#include "geom.hpp"
#include "component.hpp"
#include "ser.hpp"
#include "unit.hpp"

namespace model {

class ComponentNode;

/**
 * @brief A shared reference to a ComponentNode, this reference must
 * NOT outlive the BoardGraph that creates it as the ComponentNode shares data with 
 * the BoardGraph that will become invalidated when the BoardGraph is destructed
 */
using ComponentNodeRef = std::shared_ptr<ComponentNode>;
using WeakComponentNodeRef = std::weak_ptr<ComponentNode>;

/**
 * \brief An edge in the board graph representing a single wire connection between two
 * ports on a component
 */
class WireEdge {
public:     
    /** 
     * A \brief single connection point for a wire
     */
    struct Connection {
    public:
        /** Create a new connection point using a component and pointer to a connection port */
        inline Connection(WeakComponentNodeRef component, ConnectionPort const * port) :
            m_component{component}, m_port{port} {}
    
        /** \brief Serialize this connection point to JSON */
        json to_json() const;
    private:
        /** \brief Node in the graph that this edge connects to */
        WeakComponentNodeRef m_component;
        /** \brief Pointer to a connection port on the component node's type */
        ConnectionPort const * m_port;
        
        Connection() : m_component{}, m_port{} {}
        friend class WireEdge;
        friend class BoardGraph;
    };
    
    /** \brief Serialize this wire to JSON */
    json to_json() const;
private:
    /** \brief Components that this wire connects between*/
    std::array<Connection, 2> m_conns;

    WireEdge() : m_conns{} {};
    friend class BoardGraph;
};

using WireEdgeRef = std::shared_ptr<WireEdge>;
using WireEdgeWeakRef = std::weak_ptr<WireEdge>;

/**
 * \brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 */
class ComponentNode {
public:
    ComponentNode() : m_ty{}, m_name{}, m_pos{} {}
    
    /** \brief Get the name of this component node */
    inline constexpr const std::string& name() const { return this->m_name; }
    inline constexpr std::size_t id() const { return this->m_id; }
    
    /** \brief Fetch the underlying component type of this node */
    inline ComponentRef type() const { return this->m_ty; }
    
    ComponentNode(const std::size_t id) : m_ty{}, m_id{id}, m_name{}, m_pos{} {}

private:
    /** \brief What kind of component this is, shared with other components */
    ComponentRef m_ty;
    /** 
     * \brief The internal ID of this component node, it is not a string because it
     * is never presented to the user and is stable between runs of the program
     */
    std::size_t m_id;
    /** \brief User-assigned name of the placed part */
    std::string m_name;
    /** \brief Offset in the workspace from center */
    Point m_pos;
    /** \brief All graph edges connecting this component node to others */
    std::vector<WireEdgeRef> m_wires;

    friend class BoardGraph;
};

/**  
 * \brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
    /** \brief ID number used for nonexistent components when serializing */
    static constexpr const std::size_t NULL_ID = 0;

    /** \brief Deserialize a board graph from a JSON value */
    static void from_json(BoardGraph& self, const json& j);
    /** \brief Serialize this board graph to a JSON value */
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
    /** \brief A sparse array of internal IDs to placed components */
    compmap_type m_nodes;
    /** \brief Collection of all loaded component types */
    ComponentStore m_store;
    /** \brief ID counter for component IDs */
    std::size_t m_id_count = 1;
};

}
