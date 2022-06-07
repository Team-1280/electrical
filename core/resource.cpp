#include "resource.hpp"
#include "component.hpp"


const std::filesystem::path SharedResourceStore::COMPONENT_DIR = "./assets/";

SharedResourceStore::SharedResourceStore() : m_components{}, m_connectors{} {}

json SharedResourceStore::save_component(const Component& component) {
    json::object_t obj{};
    obj.emplace("id", component.m_id);
    obj.emplace("name", component.m_name);
    obj.emplace("footprint", component.m_fp);
    obj.emplace("ports", json::object({}));
    for(const auto& [port_id, port] : component.m_ports) {
        json::object_t port_json{};
        port_json.emplace("name", port.m_name);
        port_json.emplace("pos", port.m_pt);
        obj.emplace(port_id, port_json);
    }

    return obj;
}

Optional<Ref<Component>> SharedResourceStore::get_component(const std::string_view id) {
    const auto& existing = this->m_components.find(id);
    if(existing != this->m_components.end()) {
        return existing->second;
    }

    try {
        auto [entry, ins] = this->m_components.emplace(
            id,
            Ref<Component>{}
        );

        auto component_path = (COMPONENT_DIR / id).concat(".json");
        std::ifstream file{component_path};
        json json_val{};
        file >> json_val;
        file.close();
        
        Ref<Component> component{new Component{}};
        entry->second = component;

        component->m_id = std::string_view{entry->first};
        component->m_name = json_val.at("name");
        json_val.at("footprint").get_to<Footprint>(component->m_fp);
        for(const auto& [port_id, port_json] : json_val.at("ports").items()) {
            auto elem = component->m_ports.emplace(port_id, ConnectionPort{}).first;
                
            port_json.at("name").get_to(elem->second.m_name);
            port_json.at("pos").get_to(elem->second.m_pt);
            elem->second.m_id = std::string_view{elem->first};
        }

        return entry->second;
    } catch(const std::exception& e) {
        logger::error(
            "Failed to load component by id {}: {}",
            id,
            e.what()
        );
        return {};
    }
    return {};
}

Optional<Ref<Connector>> SharedResourceStore::get_connector(const std::string_view id) {
    const auto& existing = this->m_connectors.find(id);
    if(existing != this->m_connectors.end()) {
        return existing->second;
    }

    try {
        auto [entry, ins] = this->m_connectors.emplace(
            id,
            Ref<Connector>{}
        );

        auto component_path = (COMPONENT_DIR / id).concat(".json");
        std::ifstream file{component_path};
        json json_val{};
        file >> json_val;
        file.close();
        
        Ref<Connector> component{new Connector{}};
        entry->second = component;
        
        component->m_id = std::string_view{entry->first};
        json_val.at("name").get_to<std::string>(entry->second->m_name);
        return entry->second;
    } catch(const std::exception& e) {
        logger::error(
            "Failed to load component by id {}: {}",
            id,
            e.what()
        );
        return {};
    }
    return {};

}
