#pragma once

#include "ser/store.hpp"
#include "util/stackvec.hpp"
#include "util/freelist.hpp"
#include "util/singlevec.hpp"
#include <limits>
#include <type_traits>
#include <unit.hpp>
#include <ser/ser.hpp>
#include <utility>
#include <variant>


/**
 * \brief A 2D point on the workspace plane
 * \implements ser::FromJson
 */
struct Point {
public:
    /** Create a new point from x and y coordinate */
    constexpr Point(const Length x, const Length y) : x{x}, y{y} {}
    constexpr Point() = default; 

    /** 
     * Deserialize a point from a JSON value
     */
    static void from_json(Point& self, const json& val);
    /** Convert this point into a JSON value */
    json to_json() const;

    constexpr bool operator==(const Point& other) const = default;

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

    constexpr inline Point operator-() const {
        return Point(-this->x, -this->y);
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
 * \brief Axis-aligned bounding box that contains a minimum and maximum point, required for storage in an 
 * R-Tree and for optimizing intersection queries
 */
struct AABB {
    /**
     * Minimum x and y coordinate of the box
     * **MUST** be less than `max`
     */
    Point min;
    Point max;

    AABB() : 
        min{Length(std::numeric_limits<float>::max()), Length(std::numeric_limits<float>::max())},
        max{Length(std::numeric_limits<float>::min()), Length(std::numeric_limits<float>::min())} {}
    
    /**
     * \brief Create a new axis-aligned bounding box from minimum and maximum points
     * \param min Minimum x and y coordinate, must be less than `max`
     */
    AABB(Point&& min, Point&& max) : min{min}, max{max} {
        assert(min.x < max.x && min.y < max.y);
    }
        
    /**
     * \brief Create a new AABB with the minimum point at (0, 0) and the maximum at (width, height)
     */
    AABB(const Length& width, const Length& height) : min{Point(0._m, 0._m)}, max{Point(width, height)} {}
    
    /**
     * \brief Expand this axis-aligned bounding box to contain the given point, modifyuing `min` and `max` respectively
     * \param p The point that we must be able to contain
     */
    inline constexpr void expand(const Point& p) {
        if(p.x > this->max.x) { this->max.x = p.x; }
        else if(p.x < this->min.x) { this->min.x = p.x; }
        if(p.y > this->max.y) { this->max.y = p.y; }
        else if(p.y < this->min.y) { this->min.y = p.y; }
    }
    
    /**
     * \brief Check if a point is contained inside this bounding box
     */
    inline constexpr bool contains(const Point& point) const noexcept {
        return this->min.x <= point.x && this->min.y <= point.y &&
            this->max.x >= point.x && this->max.y >= point.y;
    }
    
    /** 
     * \brief Check if this AABB can contain another AABB
     */
    inline constexpr bool contains(const AABB& other) const noexcept {
        return this->min.x <= other.min.x && this->min.y <= other.min.y &&
            this->max.x >= other.max.x && this->max.y >= other.max.y;
    }
    
    /**
     * \brief Offset this AABB by the given x and y values
     */
    inline AABB operator+(const Point& offset) const noexcept {
        return AABB{
            Point(this->min.x + offset.x, this->min.y + offset.y),
            Point(this->max.x + offset.x, this->max.y + offset.y)
        };
    }
};

/**
 * \brief Description of a component's footprint on the 
 * workspace
 * \implements ser::JsonSerializable
 */
class Footprint {
public:
    Footprint() = default;
    
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
    Footprint(const SingleVec<Point>& pts) : Footprint(SingleVec{pts}) {}
    Footprint(SingleVec<Point>&& pts);
    
    constexpr inline operator SingleVec<Point> const&() const noexcept {
        return this->m_pts;
    }
    
    /**
     * \brief Get the axis-aligned bounding box of this footprint
     */
    inline constexpr AABB const& aabb() const noexcept { return this->m_aabb; }

    SingleVec<Point>::const_iterator begin() const { return this->m_pts.begin(); }
    SingleVec<Point>::const_iterator end() const { return this->m_pts.end(); }

private:
    /** \brief A vector of points that each connect to the prior one */
    SingleVec<Point> m_pts;
    /** \brief Axis-aligned bounding box for the footprint */
    AABB m_aabb;
        
    /** \brief Get the minimum and maximum x and y values of m_pts */
    void get_minmax();
};

static_assert(ser::JsonSerializable<Footprint>);

class ComponentNode;

