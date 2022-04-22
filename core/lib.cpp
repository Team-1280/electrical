#include "lib.hpp"

#include "util/log.hpp"

namespace model {

std::optional<ComponentNodeRef> BoardGraph::get_node(const std::string& name) {
    auto elem = this->m_nodes.find(name);
    return (elem != this->m_nodes.end()) ? elem->second : std::optional<ComponentNodeRef>{};
}

void BoardGraph::from_json(BoardGraph &self, const json &j) {
    for(const auto& [k, v] : j["nodes"].items()) {
        auto [elem, ins] = self.m_nodes.emplace(k, ComponentNodeRef{});
        if(!ins) {
            logger::warn("Failed to insert node {} by name into board graph", k);
        }
        
        elem->second->m_name = std::string_view{elem->first};
        v["pos"].get_to<Point>(elem->second->m_pos);
        std::string comp_ty_id = v["type"].get<std::string>();
        std::optional<ComponentRef> component_ty = self.m_store.find(comp_ty_id);
        if(!component_ty) {
            logger::error("Component node '{}' refers to component {} that was not found", k, comp_ty_id);
            self.m_nodes.erase(k);
            continue;
        }
        elem->second->m_ty = *component_ty;
    }
}

json BoardGraph::to_json() const {
    json::object_t obj{};
    obj["nodes"] = json::object({});
    for(const auto& [k, v] : this->m_nodes) {
        obj["nodes"][k].emplace("pos", v->m_pos);
        obj["nodes"][k].emplace("type", v->m_ty->m_name);
    }

    return obj;
}

}
