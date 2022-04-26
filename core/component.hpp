#pragma once

#include <filesystem>
#include <fstream>
#include <optional>

#include <ser.hpp>
#include <geom.hpp>
#include "util/hash.hpp"
#include "store.hpp"

namespace model {

/**
 * \brief A structure defining a single connection point on a component
 */
struct ConnectionPort {
public:
    /** Get this connection port's name */
    constexpr inline const std::string& name() const { return this->m_name; }
    /** Get this connection port's ID */
    constexpr inline const std::string_view id() const { return this->m_id; }

    inline ConnectionPort(ConnectionPort&& other) : m_pt{std::move(other.m_pt)}, m_name{std::move(other.m_name)}, m_id{std::move(other.m_id)} {}
    inline ConnectionPort& operator=(ConnectionPort&& other) {
        this->m_pt = other.m_pt;
        this->m_name = std::move(other.m_name);
        this->m_id = std::move(other.m_id);
        return *this;
    }

    ConnectionPort() : m_pt{}, m_name{}, m_id{} {}
private:
    /** Location on the component's footprint that this connection port is located */
    Point m_pt;
    /** Name of the port */
    std::string m_name;
    /** Internal ID of this connection port, shared with the parent Component and guranteed to be NULL terminated */
    std::string_view m_id;
    
    friend struct Serializer<ConnectionPort>;
    friend class Component;
};

/**
 * \brief A component in the board design with required parameters like
 * footprint
 * \sa ComponentSerializer
 * \implements GenericStoreValue
 */ 
class Component {
public:
    Component(Component&& other) : 
        m_name{other.m_name},
        m_id{other.m_id},
        m_ports{std::move(other.m_ports)},
        m_fp{std::move(other.m_fp)} {}
    Component() : m_name{}, m_id{}, m_ports{}, m_fp{} {};
    
    /** Get the name of this component */
    inline constexpr const std::string_view name() const { return this->m_name; }
    
    /** Get a port by name, O(n) lookup time */
    std::optional<std::reference_wrapper<const ConnectionPort>> get_port(const std::string& id) const;
    /** Get a port index by name */
    std::optional<std::size_t> get_port_idx(const std::string_view name) const;

private:
    /* User-facing name of the component type, shared with the ComponentStore */
    std::string_view m_name;
    /* ID string of this component, shared with the ComponentStore */
    std::string_view m_id;
    /* 
     * Map of IDs to connection points for this component 
     * Note: The WireEdge class contains pointers into this map, meaning that
     * after construction elements MUST not be removed
     */
    std::unordered_map<std::string, ConnectionPort, StringHasher, std::equal_to<>> m_ports;
    /* Shape of the component in the workspace */ 
    Footprint m_fp;
    
    friend struct Serializer<Component>;
    friend class BoardGraph;
};


}

template<>
struct Serializer<model::Component> {
    using Component = model::Component;
    using IdType = std::string;
    static const std::filesystem::path RESOURCE_DIR;
    
    template<GenericStoreValue... Resources>
    static json save(std::shared_ptr<Component>, GenericResourceManager<Resources...>&);
    static std::string load_id(const json&);
    static std::string load_name(const json&);
    template<GenericStoreValue... Resources>
    static inline std::shared_ptr<Component> load(
        const json& json_val,
        GenericResourceManager<Resources...>&,
        const std::string& idref,
        GenericStoreEntry<Component>& entry
    ) {
        std::shared_ptr<Component> component = std::make_shared<Component>();

        component->m_id = std::string_view{idref};
        component->m_name = std::string_view{entry.name};
        json_val["footprint"].get_to<model::Footprint>(component->m_fp);
        for(const auto& [port_id, port_json] : json_val["ports"].items()) {
            auto elem = component->m_ports.emplace(port_id, model::ConnectionPort{}).first;
            
            port_json["name"].get_to(elem->second.m_name);
            port_json["pos"].get_to(elem->second.m_pt);
            elem->second.m_id = std::string_view{elem->first};
        }

        return component;
    }
 
};

static_assert(GenericStoreValue<model::Component>);

