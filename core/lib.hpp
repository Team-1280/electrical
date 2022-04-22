#pragma once
#include <string> 
#include <nlohmann/json.hpp>
#include <string_view>

#include <unit.hpp>
#include <ser.hpp>
#include <component.hpp>


namespace model {

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

using ComponentNodeRef = std::shared_ptr<ComponentNode>;


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
