#include "geom.hpp"
#include <limits>
#include <lib.hpp>

void Footprint::from_json(Footprint& self, const json& val) {
    for(const json& v : val) {
        self.m_pts.push_back(v.get<Point>());
    }
    self.get_minmax();
}

json Footprint::to_json() const {
    json::array_t arr{};
    for(const Point& pt : this->m_pts) {
        arr.push_back(pt.to_json());
    }
    return arr;
}

Footprint::Footprint(std::vector<Point>&& pts) :
    m_pts{std::move(pts)},
    m_aabb{}
{
    this->get_minmax();
}

void Footprint::get_minmax() {
    for(const auto& pt : this->m_pts) {
        this->m_aabb.expand(pt);
    }

}

void Point::from_json(Point& self, const json& val) {
    val.at(0).get_to<Length>(self.x);
    val.at(1).get_to<Length>(self.y);
}

json Point::to_json() const {
    return json::array({
        this->x.to_string(),
        this->y.to_string()
    });
}

constexpr Length Point::distance(const Point &other) const {
    const LengthUnit units = this->x.unit();
    return Length(
        units,
        std::sqrt(
            std::pow(this->x.normalized() - other.x.normalized(), 2) +
            std::pow(this->y.normalized() - other.y.normalized(), 2)
        )
    );
}

BSPTree::BSPTree() {
    
}

void BSPTree::insert(const Ref<ComponentNode>& node) {
    (void)node;
    
}
