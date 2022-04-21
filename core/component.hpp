#pragma once

#include <ser.hpp>
#include <geom.hpp>

namespace model {

class ComponentId;

/**
 * @brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    /**
     * @brief Deserialize a component from a JSON value, throwing an
     * exception if the passed JSON is invalid
     */
    Component(const json& jsonval); 
        
    /** Convert this component to a JSON value */
    json to_json() const;
    
    Component(Component&& other) : 
        m_name{std::move(other.m_name)},
        m_fp{std::move(other.m_fp)} {}

private:
    //! @brief User-facing name of the component type
    std::string m_name;
    //! @brief Shape of the component in the workspace 
    Footprint m_fp;

    friend class BoardGraph;
};

static_assert(ser::JsonSerializable<Component>);

/**
 * @brief Unique identifier for a component that is managed by a ComponentStore
 */
class ComponentId {
public:
    
private:

};


/** 
 * @brief A structure storing component types with methods to lazy load
 */
class ComponentStore {
public:

private:
};



}
