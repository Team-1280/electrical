#pragma once

#include <unit.hpp>
#include <ser.hpp>

namespace model {

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

static_assert(ser::JsonSerializable<Point>);

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

static_assert(ser::JsonSerializable<Footprint>);



}