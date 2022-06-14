#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "geom.hpp"
#include "util/log.hpp"

std::optional<std::reference_wrapper<const ConnectionPort>> Component::get_port(const std::string_view id) const {
    const auto& port = this->m_ports.find(id);
    if(port != this->m_ports.end()) {
        return std::cref(port->second);
    } else {
        return std::optional<std::reference_wrapper<const ConnectionPort>>{};
    }
}

std::optional<ConnectionPortRef> Component::get_port_ref(const std::string_view id) {
    const auto& port = this->m_ports.find(id);
    if(port != this->m_ports.end()) {
        return &port->second;
    } else {
        return {};
    }
}

std::filesystem::path ComponentLoader::DIR = "./assets/components";

Ref<Component> ComponentLoader::load(std::string_view id, const json &json_val, LazyResourceStore &store) {
    (void)store;
    Ref<Component> component{new Component{}};
    component->m_id = id;
    json_val.at("name").get_to<std::string>(component->m_name);
    json_val.at("footprint").get_to<Footprint>(component->m_fp);
    for(const auto& [port_id, port_json] : json_val.at("ports").items()) {
        auto elem = component->m_ports.emplace(port_id, ConnectionPort{}).first;
            
        port_json.at("name").get_to<std::string>(elem->second.m_name);
        port_json.at("pos").get_to<Point>(elem->second.m_pt);
        elem->second.m_id = std::string_view{elem->first};
    }

    return component;
}
