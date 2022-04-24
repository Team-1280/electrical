#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "util/log.hpp"

namespace model {

const std::filesystem::path ComponentStore::CACHEFILE_PATH = "./assets/components/cache.json";
const std::filesystem::path ComponentStore::COMPONENT_DIR = "./assets/components/";

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
        for(const auto& [port_id, port] : json_val["ports"].items()) {
            auto [elem, inserted] = ref->m_ports.emplace(port_id, ConnectionPort{});
            if(!inserted) {
                logger::warn("Component {} has multiple definitions for port {}", ref->m_id, port_id);
                continue;
            }
            
            elem->second.m_id = std::string_view{elem->first};
            port["name"].get_to<std::string>(elem->second.m_name);
            port["pos"].get_to<Point>(elem->second.m_pt);
        }
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

std::optional<std::reference_wrapper<const ConnectionPort>> Component::get_port(const std::string& id) const {
    const auto& port = this->m_ports.find(id);
    if(port != this->m_ports.end()) {
        return std::cref(port->second);
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

}
