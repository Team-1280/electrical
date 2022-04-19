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

using json = nlohmann::json;

namespace model {

/**
 * @brief A unique identifier for a component type, internally containing
 * the hash of a string
 */
struct id {
    inline constexpr id() : m_n{0} {}
    /** Create a new identifier from a raw number */
    inline constexpr id(const std::uint64_t n) : m_n{n} {}
    
    /**
     * @brief Create a new ID from the FNV-1A hash of a string
     */
    constexpr id(const std::string_view str);

    json to_json() const;

    /**
     * @brief Create a new ID from the FNV-1A hash of the given NULL-terminated
     * string
     */
    constexpr id(std::uint8_t const * str);

    inline constexpr id(const id& other) : m_n{other.m_n} {}
    inline constexpr auto operator <=>(const id& other) const = default;

private:
    std::uint64_t m_n;
};

/**
 * @brief A 2D point on the workspace plane
 */
struct Point {
public:
    /** Create a new point from x and y coordinate */
    constexpr Point(const Length x, const Length y) : x{x}, y{y} {}
        
    /** 
     * @brief Deserialize a point from a JSON value
     */
    Point(const json& val);
    /** Convert this point into a JSON value */
    json to_json() const;

    constexpr bool operator==(const Point& other) const {
        return other.x == this->x && other.y == this->y;
    }
    
    /**
     * @brief Get the distance between two points, returning a distance in the units
     * of this's x coordinate
     */
    constexpr Length distance(const Point& other) const;

    Length x;
    Length y;
};


/**
 * @brief Description of a component's footprint on the 
 * workspace
 */
class Footprint {
public:

    Footprint() = default;
    
    /** Create a new footprint from the JSON array */
    Footprint(const json& json);
        
    /** Convert this footprint to a JSON array */
    json to_json() const;

    /**
     * @brief Create a new footprint from a list of connected points
     */
    Footprint(const std::vector<Point>& pts) : m_pts{pts} {}
    Footprint(std::vector<Point>&& pts) : m_pts{std::move(pts)} {}
    
    constexpr operator std::vector<Point>&() {
        return this->m_pts;
    }

private:
    /** A vector of points that each connect to the prior one*/
    std::vector<Point> m_pts;
};


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
        m_id{other.m_id},
        m_name{std::move(other.m_name)},
        m_fp{std::move(other.m_fp)} {}

private:
    //! @brief Unique identifier of this component type used to reference it in the model
    id m_id;
    //! @brief User-facing name of the component type
    std::string m_name;
    //! @brief Shape of the component in the workspace 
    Footprint m_fp;

    friend class BoardGraph;
};

/** 
 * @brief A structure storing component types with methods to lazy load
 */
class ComponentStore {

};


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
        id comp_id;
    };
    
    /**
     * @brief An arena containing all nodes with their counts
     */
    std::vector<Node> m_nodes;
};

}
