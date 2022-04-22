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

private:
    /** What kind of component this is, shared with other components */
    ComponentRef m_ty;
    /** User-assigned name of the placed part */
    std::string m_name;
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
    
private:
    /** Collection of all loaded component types */
    ComponentStore m_store;
};

}
