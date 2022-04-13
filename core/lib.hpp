#pragma once
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace model {

using id = std::uint64_t;

/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    
private:
    const id m_id;
};

}
