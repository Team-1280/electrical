#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "util/log.hpp"

namespace model {

const std::filesystem::path ComponentSerializer::RESOURCE_DIR = "./assets/components/";

json ConnectionPort::to_json() const {
    return {
        {"name", this->m_name},
        {"id", this->m_id},
        {"pos", this->m_pt}
    };
}

json ComponentSerializer::save(std::shared_ptr<Component> component, [[maybe_unused]] ComponentSerializer::Store& store) {
    json::object_t obj{};
    obj.emplace("id", component->m_id);
    obj.emplace("name", component->m_name);
    obj.emplace("footprint", component->m_fp);
    obj.emplace("ports", json::object({}));
    for(const auto& [port_id, port]: component->m_ports) {
        obj["ports"].emplace(port_id, json::object({
            {"name", port.m_name},
            {"pos", port.m_pt}
        }));
    }
    return obj;
}

std::string ComponentSerializer::load_id(const json& j) {
    return j["id"];
}

std::string ComponentSerializer::load_name(const json& j) {
    return j["name"];
}

std::shared_ptr<Component> ComponentSerializer::load(
        const json& json_val,
        [[maybe_unused]] ComponentSerializer::Store& store,
        const std::string& idref,
        GenericStoreEntry<Component>& entry
    ) {
    std::shared_ptr<Component> component = std::make_shared<Component>();

    component->m_id = std::string_view{idref};
    component->m_name = std::string_view{entry.name};
    json_val["footprint"].get_to<Footprint>(component->m_fp);
    for(const auto& [port_id, port_json] : json_val["ports"].items()) {
        auto elem = component->m_ports.emplace(port_id, ConnectionPort{}).first;
        
        port_json["name"].get_to(elem->second.m_name);
        port_json["pos"].get_to(elem->second.m_pt);
        elem->second.m_id = std::string_view{elem->first};
    }

    return component;
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
