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
    //! @brief Hash of the ID string required in the component file
    // used to reference the component type
    const id m_id;
    //! @brief User-facing name of the component
    std::string m_name;
    //! @brief 
    
};

}
