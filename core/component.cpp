#include <component.hpp>
#include <fstream>
#include "util/log.hpp"

namespace model {

const std::filesystem::path ComponentStore::CACHEFILE_PATH = "./assets/components/cache.json";
const std::filesystem::path ComponentStore::COMPONENT_DIR = "./assets/components/";

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
        ref->m_fp = Footprint{json_val["footprint"]};
    } catch(std::exception e) {
        logger::error("Failed to load component from {}: {}", stored->second.path, e.what());
        return std::optional<ComponentRef>{};
    }
    
    logger::trace("Loaded component {}", id);
    stored->second.loaded = std::weak_ptr{ref};
    return ref;
}

ComponentStore::StoreEntry::StoreEntry(const json& j) : loaded{} {
    this->name = j["name"];
    this->path = j["path"];
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
