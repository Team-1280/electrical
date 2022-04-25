#include "lib.hpp"
#include <unordered_set>

namespace model {

json WireEdge::Connection::to_json() const {
    if(this->m_component.expired()) {
        return nullptr;
    } else {
        return {
            {
                "node", 
                this->m_component.lock()->id(),
            },
            {
                "port",
                this->m_port->id()
            }
        };
    }
}

json WireEdge::to_json() const {
    return {this->m_conns[0].to_json(), this->m_conns[1].to_json()};
}

void BoardGraph::from_json(BoardGraph &self, const json &j) {
    for(const auto& val : j["nodes"]) {
        std::size_t id = val["id"].get<std::size_t>();
        auto [elem, ins] = self.m_nodes.emplace(
            id,
            std::make_shared<ComponentNode>(id)
        );
        if(!ins) {
            logger::warn("Failed to insert node {} by ID into board graph", id);
        }
        
        val["name"].get_to<std::string>(elem->second->m_name);;
        val["pos"].get_to<Point>(elem->second->m_pos);
        std::string comp_ty_id = val["type"].get<std::string>();
        std::optional<ComponentRef> component_ty = self.m_store.get(std::string_view{comp_ty_id});
        if(!component_ty) {
            logger::error("Component node '{}' refers to component {} that was not found", id, comp_ty_id);
            self.m_nodes.erase(id);
            continue;
        }
        elem->second->m_ty = *component_ty;
    }

    for(const auto& edge : j["edges"]) {
        WireEdgeRef wire{new WireEdge{}};

        for(std::size_t i = 0; i < 2; ++i) {
            //Ignore nonexistent connection points
            if(edge[i].is_null()) {
                continue; 
            }
            std::size_t component_id = edge[i]["node"].get<std::size_t>();
            auto component = self.m_nodes.find(component_id);
            if(component == self.m_nodes.end()) {
                logger::warn("Edge connects to nonexistent component node {}", component_id);
                continue;
            }
            std::string port_id = edge[i]["port"].get<std::string>();
            auto port = component->second->m_ty->m_ports.find(port_id);
            if(port == component->second->m_ty->m_ports.end()) {
                logger::warn(
                    "Edge connects to valid node {} to invalid port {} for component type {}",
                    component_id,
                    port_id,
                    component->second->m_ty->m_id
                );
                continue;
            }
            wire->m_conns[i].m_component = component->second;
            wire->m_conns[i].m_port = &port->second;
            component->second->m_wires.push_back(wire);
        }
    }
}

json BoardGraph::to_json() const {
    json::object_t obj{};
    obj["nodes"] = json::array({});
    obj["edges"] = json::array({});
    std::unordered_set<WireEdgeRef> serialized_edges{};

    for(const auto& [k, v] : this->m_nodes) {
        json::object_t node{};
        node.emplace("pos", v->m_pos);
        node.emplace("type", v->m_ty->m_id);
        node.emplace("name", v->name());

        for(const auto& edge : v->m_wires) {
            if(serialized_edges.contains(edge)) {
                continue;
            } 
            serialized_edges.emplace(edge);

            obj["edges"].push_back(edge->to_json());
        }
        obj["nodes"].push_back(node);
    }
    
    return obj;
}

}
