#pragma once

#include "ser/store.hpp"
#include "util/stackvec.hpp"
#include "util/freelist.hpp"
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
    Footprint(const std::vector<Point>& pts) : Footprint(std::vector{pts}) {}
    Footprint(std::vector<Point>&& pts);
    
    constexpr operator std::vector<Point> const&() const noexcept {
        return this->m_pts;
    }
    
    /**
     * \brief Get the axis-aligned bounding box of this footprint
     */
    inline constexpr AABB const& aabb() const noexcept { return this->m_aabb; }

    std::vector<Point>::const_iterator begin() const { return this->m_pts.begin(); }
    std::vector<Point>::const_iterator end() const { return this->m_pts.end(); }

private:
    /** \brief A vector of points that each connect to the prior one, must have at least one point */
    std::vector<Point> m_pts;
    /** \brief Axis-aligned bounding box for the footprint */
    AABB m_aabb;
        
    /** \brief Get the minimum and maximum x and y values of m_pts */
    void get_minmax();
};

static_assert(ser::JsonSerializable<Footprint>);

class ComponentNode;

/**
 * \brief Data structure that contains multiple `Footprint`'s efficiently divided into
 * two subtrees, providing performant operations like nearest neighbor and Point In Polygon
 * \sa https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det
 */
class BSPTree {
public:
    /** Using 16 bit IDs for nodes reduces their size to 8 bytes, and limits our maximum depth to 16 */
    using size_type = std::uint16_t;
    /** Index value representing an invalid index */
    static constexpr const size_type npos = std::numeric_limits<size_type>::max();
    /** \brief The maximum depth of recursion for this BSP tree */
    static constexpr const std::size_t MAX_DEPTH = 16;
    /** \brief Maximum number of elements to insert into a single node before splitting it */
    static constexpr const std::size_t MAX_ELEMS = 32;
        
    /**
     * \brief A single element in the FreeList of elements, containing an index to the next 
     * element and the element data
     */
    struct Element {
        /** \brief Reference to the node in the board graph */
        WeakRef<ComponentNode> data;
        /** \brief Index of the next element in this linked list of elements */
        FreeList<Element>::size_type next{FreeList<Element>::npos};

        Element(WeakRef<ComponentNode>&& d) : data{d} {}
    };

    using ElementList = FreeList<Element>;

    /**
     * \brief A node that may contain either an index to 4 more child nodes or an index to 
     */
    struct Node {
        /** \brief Index of the left node, or `npos` if it doesn't exist */
        size_type left{npos};
        /** \brief Index of the right node, or `npos` if it doesn't exist */
        size_type right{npos};
        /** \brief Index of the first element stored in this node */
        ElementList::size_type elems{ElementList::npos};
    };
    
    /** \brief Create a new BSP tree with the given maximum size */
    BSPTree(const AABB& sz);
    BSPTree(const BSPTree& other) = default;
    BSPTree(BSPTree&& other) = default;
    BSPTree& operator=(BSPTree&& other) = default;
    
    /**
     * \brief Insert node into this BSP tree, using its component's footprint offset by
     * the position of the node
     */
    void insert(const Ref<ComponentNode>& node);
private:
    /** 
     * \brief List of all nodes in this tree, stored as a flat vector with indices instead of pointers.
     * The first element is always the root element
     */
    FreeList<Node> m_nodes{};
    /** \brief List of all elements in this tree, stored indices in nodes point to this */
    FreeList<Element> m_elems{};
    /** \brief Size of this tree, changing this requires rebuilding the tree */
    Length::Raw m_sizex, m_sizey{};
    
    /** 
     * \brief Insert a value into this tree
     * \param value The value to insert
     * \param node The node to insert a value into
     * \param midx X midpoint of the node to insert
     * \param midy Y midpoint of the node to insert
     * \param depth The current recursion depth
     */
    void insert(ElementList::size_type val, Node node, Length::Raw midx, Length::Raw midy, size_type depth);
    
    /**
     * \brief Add an element to the given node
     */
    void add_elem(ElementList::size_type val, Node node);

    void remove_elem(ElementList::size_type val, Node node);
    
    /**
     * \brief Get the number of elements that a given node contains
     */
    std::size_t node_elems(Node node) const;
};
