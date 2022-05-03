#include <component.hpp>
#include <exception>
#include <fstream>
#include <stdexcept>

#include "util/log.hpp"

namespace model {

std::optional<std::reference_wrapper<const ConnectionPort>> Component::get_port(const std::string_view id) const {
    const auto& port = this->m_ports.find(id);
    if(port != this->m_ports.end()) {
        return std::cref(port->second);
    } else {
        return std::optional<std::reference_wrapper<const ConnectionPort>>{};
    }
}

std::optional<ConnectionPortRef> Component::get_port_ptr(const std::string_view id) {
    const auto& port = this->m_ports.find(id);
    if(port != this->m_ports.end()) {
        return &port->second;
    } else {
        return {};
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

const std::filesystem::path ResourceSerializer<model::Component>::RESOURCE_DIR = "./assets/components";
