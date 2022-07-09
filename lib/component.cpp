#include <algorithm>
#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "data.hpp"
#include "geom.hpp"
#include "util/log.hpp"


Optional<std::reference_wrapper<const ConnectionPort>> Component::get_port(const std::string_view id) const {
    const auto& port = std::find_if(
        this->m_ports.cbegin(),
        this->m_ports.cend(),
        [&id](auto const& port) { return port.name() == id; }
    );
    if(port != this->m_ports.cend()) {
        return std::cref(*port);
    } else {
        return Optional<std::reference_wrapper<const ConnectionPort>>{};
    }
}

Optional<ConnectionPortIdx> Component::get_port_idx(const std::string_view id) const {
    const auto& port = std::find_if(
        this->m_ports.cbegin(),
        this->m_ports.cend(),
        [&id](auto const& port) { return port.name() == id; }
    );
    if(port != this->m_ports.end()) {
        return port.index();
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
    if(json_val.contains("mass")) {
        json_val.at("mass").get_to<Optional<Mass>>(component->m_mass);
    }

    if(json_val.contains("purchase")) {
        json_val.at("purchase").get_to<Optional<PurchaseData>>(component->m_purchasedata);
    }
    for(const auto& [port_id, port_json] : json_val.at("ports").items()) {
        auto elem = component->m_ports.emplace(ConnectionPort{});
            
        port_json.at("name").get_to<std::string>(component->m_ports[elem].m_name);
        port_json.at("pos").get_to<Point>(component->m_ports[elem].m_pt);
        component->m_ports[elem].m_id = port_id;
    }

    return component;
}
