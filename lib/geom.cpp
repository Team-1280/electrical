#include "geom.hpp"
#include <cstddef>
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

BSPTree::BSPTree(const AABB& aabb) : 
    m_sizex{(aabb.max.x - aabb.min.x).normalized()},
    m_sizey{(aabb.max.y - aabb.min.y).normalized()}
{
    this->m_nodes.emplace(Node {
        .left = npos,
        .right = npos,
        .elems = ElementList::npos,
    }); 
}


/** Enumeration of how an AABB may be contained in a BSP tree node */
enum class Contained {
    Left,
    Right,
    Across
};

void BSPTree::insert(const Ref<ComponentNode> &node) {
    fmt::print("STARTED INSERTION\n");
    this->insert(
        this->m_elems.emplace(node),
        this->m_nodes[0],
        this->m_sizex / 2.,
        this->m_sizey / 2.,
        1,
        0
    );
}

void BSPTree::insert(ElementList::size_type elem, Node& node, Length::Raw midx, Length::Raw midy, size_type depth, size_type id) {
    fmt::print("Node 0 has l={}, r={}\n", this->m_nodes[0].left, this->m_nodes[0].right);
    if(depth >= MAX_DEPTH) {
        this->add_elem(elem, node);
        return;
    }
    const auto& val = this->m_elems[elem].data.lock();
    Contained contained;
    if(depth & 1) {
        if(val->aabb().min.x.normalized() < midx) {
            if(val->aabb().max.x.normalized() < midx) { contained = Contained::Left; }
            else { contained = Contained::Across; }
        } else if(val->aabb().max.x.normalized() >= midx) { contained = Contained::Right; }
        else { contained = Contained::Across; }
    } else {
        if(val->aabb().min.y.normalized() < midy) {
            if(val->aabb().max.y.normalized() < midy) { contained = Contained::Left; }
            else { contained = Contained::Across; } 
        } else if(val->aabb().max.y.normalized() >= midy) { contained = Contained::Right; }
        else { contained = Contained::Across; }
    }
    
    if(contained == Contained::Across) {
        this->add_elem(elem, node);
        return;
    }

    size_type& insert_node = (contained == Contained::Right) ? node.right : node.left;
    fmt::print("insert element {}, node {} from {} (with r={}, l={}), right: {}\n", elem, insert_node, id, node.right, node.left, contained == Contained::Right);
    if(insert_node != npos) {
        fmt::print("Descending to node {} from {}, depth={}\n", insert_node, id, depth);
        this->insert(
            elem,
            this->m_nodes[insert_node],
            (depth & 1) ? midx : midx / 2.,
            (depth & 1) ? midy / 2. : midy,
            depth + 1,
            insert_node
        );
    } else if(node_elems(node) >= MAX_ELEMS) {
        fmt::print("Splitting {}\n", node.elems);
        insert_node = this->m_nodes.emplace();
        fmt::print("new node has id {}, left={}, right={}\n", insert_node, node.left, node.right);
        ElementList::size_type child = node.elems;
        node.elems = ElementList::npos;
        while(child != ElementList::npos) {
            if(this->m_elems[child].data.expired()) {
                auto next = this->m_elems[child].next;
                this->m_elems.erase(child);
                child = next;
                continue;
            }
            fmt::print("INSERTING AFTER SPLIT, next={}\n", this->m_elems[child].next);
            auto next = this->m_elems[child].next;
            this->m_elems[child].next = ElementList::npos;
            this->insert(child, node, midx, midy, depth, id);
            child = next;
        }

        fmt::print("DONE\n");
        this->insert(
            elem,
            node,
            midx,
            midy,
            depth,
            id
        );
    } else {
        fmt::print("Adding to node {}\n", node.elems);
        this->add_elem(elem, node);
    }
}

void BSPTree::add_elem(ElementList::size_type val, Node& node) {
    if(node.elems != ElementList::npos) {
        ElementList::size_type next_open = node.elems;
        for(;;) {
            auto next = this->m_elems[next_open].next;
            if(next == ElementList::npos) {
                break;
            } else {
                next_open = next;
            }
        }
        this->m_elems[next_open].next = val;
    } else {
        node.elems = val;
    }
}

std::size_t BSPTree::node_elems(Node node) const {
    std::size_t count = 0;
    auto next = node.elems;
    while(next != ElementList::npos) {
        next = this->m_elems[next].next;
        count += 1;
    }
    return count;
}
