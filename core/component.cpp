#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>
#include "util/log.hpp"

namespace model {

const std::filesystem::path ComponentStore::CACHEFILE_PATH = "./assets/components/cache.json";
const std::filesystem::path ComponentStore::COMPONENT_DIR = "./assets/components/";


json Component::to_json() const {
    json obj = json::object({
        {"name", this->m_name},
        {"id", this->m_id},
        {"footprint", this->m_fp},
        {"conns", json::array({})}
    });

    for(auto& [k, v] : this->m_conns) {
        obj["conns"].push_back({
            {"name", k},
            {"pos", v.m_pt}
        });
    }
    return obj;
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
        for(const json& conn : json_val["conns"]) {
            std::string name = conn["name"].get<std::string>();
            ref->m_conns.insert(
                std::make_pair<std::string, ConnectionPort>(std::string{name}, ConnectionPort{})
            );
            auto& [k, v] = *ref->m_conns.find(name);
            conn["pos"].get_to<Point>(v.m_pt);
            v.m_name = std::string_view(k); //Share the key string with the Connectionport to avoid allocating twice
        }
    } catch(json::exception& e) {
        logger::error("Failed to load component from {}: {}", stored->second.path, e.what());
        return std::optional<ComponentRef>{};
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
        self.m_store[id] = ComponentStore::StoreEntry{val};
    }
}

ComponentStore::ComponentStore() {
    std::ifstream cache{CACHEFILE_PATH};
    json j;
    cache >> j;
    from_json(*this, j);
}

}
