#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string> 
#include <nlohmann/json.hpp>
#include <string_view>
#include <type_traits>
#include <vector>
#include <concepts>

#include <unit.hpp>
#include <ser.hpp>
#include <component.hpp>


namespace model {

/**  
 * @brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
    /** A reference to a component in this graph */
    struct NodeRef {
        NodeRef() = delete;
        constexpr inline size_t idx() const { return this->m_idx; }
    private:
        NodeRef(const size_t gen, const size_t idx) : m_gen{gen}, m_idx{idx} {}

        const size_t m_gen;
        const size_t m_idx;
        friend class BoardGraph;
    };
    
    /** 
     * @brief Add a component to this graph
     * @return A reference to the added node in this graph
     */
    const NodeRef add(Component&& comp);

private:
    struct Node {
        size_t count;
    };
    
    /**
     * @brief An arena containing all nodes with their counts
     */
    std::vector<Node> m_nodes;
};

}
