#pragma once

#include <unit.hpp>
#include <ser.hpp>

namespace model {

/**
 * \brief A 2D point on the workspace plane
 * \implements ser::FromJson
 */
struct Point {
public:
    /** Create a new point from x and y coordinate */
    constexpr Point(const Length x, const Length y) : x{x}, y{y} {}
    Point() : x{}, y{} {}; 

    /** 
     * Deserialize a point from a JSON value
     */
    static void from_json(Point& self, const json& val);
    /** Convert this point into a JSON value */
    json to_json() const;

    constexpr bool operator==(const Point& other) const {
        return other.x == this->x && other.y == this->y;
    }

    constexpr inline Point operator*(const Point& other) const {
        return Point{this->x * other.x, this->y * other.y};
    }
    constexpr inline Point operator/(const Point& other) const {
        return Point{this->x / other.x, this->y / other.y};
    }
    constexpr inline Point operator+(const Point& other) const {
        return Point{this->x + other.x, this->y + other.y};
    }
    constexpr inline Point operator-(const Point& other) const {
        return Point{this->x - other.x, this->y - other.y};
    }

    constexpr inline Point& operator+=(const Point& other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }
    constexpr inline Point& operator-=(const Point& other) {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    } 
    constexpr inline Point& operator*=(const Point& other) {
        this->x *= other.x;
        this->y *= other.y;
        return *this;
    }
    constexpr inline Point& operator/=(const Point& other) {
        this->x /= other.x;
        this->y /= other.y;
        return *this;
    }

    constexpr inline Point operator*(float scale) const {
        return Point(this->x * scale, this->y * scale);
    }
    constexpr inline Point& operator*=(float scale) {
        this->x *= scale;
        this->y *= scale;
        return *this;
    }

    constexpr inline Point operator/(float scale) const {
        return Point(this->x / scale, this->y / scale);
    }
    constexpr inline Point& operator/=(float scale) {
        this->x /= scale;
        this->y /= scale;
        return *this;
    }

    /**
     * \brief Get the distance between two points, returning a distance in the units
     * of this's x coordinate
     */
    constexpr Length distance(const Point& other) const;

    Length x;
    Length y;
};

static_assert(ser::JsonSerializable<Point>);

/**
 * \brief Description of a component's footprint on the 
 * workspace
 * \implements ser::JsonSerializable
 */
class Footprint {
public:

    Footprint() : m_pts{} {};
    
    /** \brief Create a new footprint from the JSON array */
    static void from_json(Footprint& self, const json& json);
        
    /** \brief Convert this footprint to a JSON array */
    json to_json() const;

    /**
     * \brief Get the first point in this footprint, guranteed to be 
     * available
     */
    inline const Point& first() const { return this->m_pts[0]; }

    /**
     * \brief Create a new footprint from a list of connected points
     */
    Footprint(const std::vector<Point>& pts) : m_pts{pts} {}
    Footprint(std::vector<Point>&& pts) : m_pts{std::move(pts)} {}
    
    constexpr operator std::vector<Point>&() {
        return this->m_pts;
    }

    std::vector<Point>::const_iterator begin() const { return this->m_pts.begin(); }
    std::vector<Point>::const_iterator end() const { return this->m_pts.end(); }

private:
    /** \brief A vector of points that each connect to the prior one*/
    std::vector<Point> m_pts;
};

static_assert(ser::JsonSerializable<Footprint>);



}
