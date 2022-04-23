#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "util/log.hpp"

namespace model {

const std::filesystem::path ComponentStore::CACHEFILE_PATH = "./assets/components/cache.json";
const std::filesystem::path ComponentStore::COMPONENT_DIR = "./assets/components/";

void ConnectionPort::from_json(ConnectionPort &self, const json &j) {
    j["name"].get_to<std::string>(self.m_name);
    j["id"].get_to<std::string>(self.m_id);
    j["pos"].get_to<Point>(self.m_pt);
}

json ConnectionPort::to_json() const {
    return {
        {"name", this->m_name},
        {"id", this->m_id},
        {"pos", this->m_pt}
    };
}

json Component::to_json() const {
    return {
        {"name", this->m_name},
        {"id", this->m_id},
        {"footprint", this->m_fp},
        {"ports", this->m_ports}
    };
}

json PortRef::to_json() const {
    if(this->m_ref.expired()) {
        logger::warn("Attempting to serialize port reference to a component that doesn't exist");
        return {};
    } 
    ComponentNodeRef locked = this->m_ref.lock();
    return json::array({
        locked->id(), locked->type()->get_port(this->m_idx).name()
    });
}

std::optional<ComponentRef> ComponentStore::find(const std::string& id) {
    auto stored = this->m_store.find(id);
    if(stored == this->m_store.end()) {
        logger::warn("Component referenced by ID {} not found", id);
        return std::optional<ComponentRef>{};
    }


    if(!stored->second.loaded.expired()) {
        return std::shared_ptr{stored->second.loaded};
    }

    json json_val;
    ComponentRef ref = std::make_shared<Component>();
    try {
        std::ifstream file{COMPONENT_DIR / stored->second.path};
        file >> json_val;
        ref->m_id = std::string_view{stored->first};
        ref->m_name = std::string_view{stored->second.name};
        json_val["footprint"].get_to<Footprint>(ref->m_fp);
        json_val["ports"].get_to<std::vector<ConnectionPort>>(ref->m_ports);
    } catch(std::exception& e) {
        logger::error("Failed to load component from {}: {}", stored->second.path, e.what());
        return std::optional<ComponentRef>{};
    }
    logger::trace("Loaded component {}", id);
    stored->second.loaded = std::weak_ptr{ref};
    return ref;
}

void ComponentStore::StoreEntry::from_json(ComponentStore::StoreEntry& self, const json& j) {
    self.name = j["name"].get<std::string>();
    self.path = j["path"].get<std::string>();
}

json ComponentStore::StoreEntry::to_json() const {
    return {
        {"name", this->name},
        {"path", this->path}
    };
}

void ComponentStore::from_json(ComponentStore& self, const json& j) {
    for(const auto& [id, val] : j.items()) {
        if(self.m_store.contains(id)) {
            logger::warn("Component store contains two cached entries with ID {}", id);
        }
        self.m_store.emplace(id, val);
    }
}

ComponentStore::ComponentStore() {
    std::ifstream cache{CACHEFILE_PATH};
    json j;
    cache >> j;
    from_json(*this, j);
}

std::optional<std::size_t> Component::get_port_idx(const std::string_view id) const {
    for(std::size_t i = 0; i < this->m_ports.size(); ++i) {
        if(this->m_ports[i].id() == id) {
            return i;
        }
    }
    return std::optional<std::size_t>{};
}

std::optional<std::reference_wrapper<const ConnectionPort>> Component::get_port(const std::string_view id) const {
    std::optional<std::size_t> idx = this->get_port_idx(id);
    if(idx.has_value()) {
        return std::cref(this->m_ports[*idx]);
    } else {
        return std::optional<std::reference_wrapper<const ConnectionPort>>{};
    }
}

constexpr const std::uint64_t FNV_PRIME = 1099511628211ULL;
constexpr const std::uint64_t FNV_OFFSET = 14695981039346656037ULL;

constexpr std::uint64_t fnv1a(const std::string_view str) {
    std::uint64_t hash = FNV_OFFSET;
    for(const std::uint8_t c : str) {
        hash = (hash ^ c) * FNV_PRIME;
    }

    return hash;
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
        std::optional<ComponentRef> component_ty = self.m_store.find(comp_ty_id);
        if(!component_ty) {
            logger::error("Component node '{}' refers to component {} that was not found", id, comp_ty_id);
            self.m_nodes.erase(id);
            continue;
        }
        elem->second->m_ty = *component_ty;
        elem->second->m_conns.resize((*component_ty)->m_ports.size());
    }
    
    //Resolve component node connections now that we have loaded all component nodes
    //by name
    for(auto const& [k, v] : j["nodes"].items()) {
        std::size_t id = v["id"].get<std::size_t>();
        //Skip nodes that we errored out on before
        if(!self.m_nodes.contains(id)) continue;
        ComponentNodeRef from = self.m_nodes[id];
        for(auto const& [from_port_id, conn] : v["conns"].items()) {
            try {
                std::size_t from_id = conn[0].get<std::size_t>();
                std::string to_port_id = conn[1].get<std::string>();

                const auto to = self.m_nodes.find(from_id);
                if(to == self.m_nodes.end()) {
                    logger::error("Component node {} connects to non-existent node {}, port {}", id, from_id, to_port_id);
                    continue;
                }
                std::optional<std::size_t> from_port_idx = from->m_ty->get_port_idx(from_port_id);
                if(!from_port_idx.has_value()) {
                    logger::error(
                        "Component node {} has a connection from port {} that doesn't exist in component type {}",
                        from->name(),
                        from_port_id,
                        from->type()->name()
                    );
                    continue;
                }

                std::optional<std::size_t> to_port_idx = to->second->m_ty->get_port_idx(to_port_id);
                if(!to_port_idx.has_value()) {
                    logger::error(
                        "Component node {} connects to non-existent port {} in component type {}",
                        from->name(),
                        to_port_id,
                        to->second->m_ty->name()
                    );
                    continue;
                }

                from->m_conns[*from_port_idx] = PortRef{WeakComponentNodeRef{to->second}, *to_port_idx};
                to->second->m_conns[*to_port_idx] = PortRef{WeakComponentNodeRef{from}, *from_port_idx};
            } catch(std::exception& e) {
                logger::error("Failed to deserialize connection from port {} for component {}[{}]: {}", from_port_id, from->id(), from->name(), e.what());
                continue;
            }
        }
    }
}

json BoardGraph::to_json() const {
    json::object_t obj{};
    obj["nodes"] = json::array({});
    for(const auto& [k, v] : this->m_nodes) {
        json::object_t node{};
        node.emplace("pos", v->m_pos);
        node.emplace("type", v->m_ty->m_id);
        node.emplace("name", v->name());
        node.emplace("conns", json::object({}));
        for(std::size_t port_idx = 0; port_idx < v->m_conns.size(); ++port_idx) {
            if(v->m_conns[port_idx].has_value()) {
                node["conns"].emplace(v->type()->get_port(port_idx).name(), v->m_conns[port_idx]->to_json());
            }
        }
        obj["nodes"].push_back(node);
    }

    return obj;
}

}
