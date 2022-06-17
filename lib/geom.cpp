#include "geom.hpp"
#include <limits>

void Footprint::from_json(Footprint& self, const json& val) {
    for(const json& v : val) {
        self.m_pts.push_back(v.get<Point>());
    }
}

json Footprint::to_json() const {
    json::array_t arr{};
    for(const Point& pt : this->m_pts) {
        arr.push_back(pt.to_json());
    }
    return arr;
}

bool Footprint::contains_aabb(const Point &p) const {
    return 
        (this->m_min.x <= p.x && this->m_min.y <= p.y) &&
        (this->m_max.x >= p.x && this->m_max.y >= p.y);
}

Footprint::Footprint(std::vector<Point>&& pts) :
    m_pts{std::move(pts)},
    m_max{Length{std::numeric_limits<float>::min()}, Length{std::numeric_limits<float>::min()}},
    m_min{Length{std::numeric_limits<float>::max()}, Length{std::numeric_limits<float>::max()}}
{
    for(const auto& pt : pts) {
        if(pt.x < this->m_min.x) { this->m_min.x = pt.x; }
        else if(pt.y < this->m_min.y) { this->m_min.y = pt.y; }
        else if(pt.x > this->m_max.x) { this->m_max.x = pt.x; }
        else if(pt.y > this->m_max.y) { this->m_max.y = pt.y; }
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
