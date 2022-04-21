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
    
private:
    ComponentStore m_store;
};

}
