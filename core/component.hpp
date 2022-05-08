#pragma once

#include <filesystem>
#include <fstream>
#include <optional>

#include <ser.hpp>
#include <geom.hpp>
#include "util/hash.hpp"
#include "store.hpp"

namespace model {

class Component;

/**
 * \brief A structure defining a single connection point on a component
 */
struct ConnectionPort {
public:
    /** Get this connection port's name */
    constexpr inline const std::string& name() const { return this->m_name; }
    /** Get this connection port's ID */
    constexpr inline const std::string_view id() const { return this->m_id; }
    /** Get the offset from the component base of this connection port */
    constexpr inline const Point& pos() const { return this->m_pt; }

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
    
    friend struct ResourceSerializer<Component>;
    friend class Component;
};

using ConnectionPortRef = const ConnectionPort *;

/**
 * \brief A component in the board design with required parameters like
 * footprint
 *
 * \implements Resource
 */ 
class Component {
public:
    using port_map_type = std::unordered_map<std::string, ConnectionPort, StringHasher, std::equal_to<>>;
    Component(Component&& other) : 
        m_name{other.m_name},
        m_id{other.m_id},
        m_ports{std::move(other.m_ports)},
        m_fp{std::move(other.m_fp)} {}
    Component() : m_name{}, m_id{}, m_ports{}, m_fp{} {};
    
    /** Get the name of this component */
    inline constexpr const std::string_view name() const { return this->m_name; }
    /** Get the user-assigned ID of this component */
    inline constexpr std::string_view id() const { return this->m_id; }
    /** Get a reference to this component's footprint */
    constexpr inline const Footprint& footprint() const { return this->m_fp; }
    
    /** Get a port by name, O(1) lookup time */
    std::optional<std::reference_wrapper<const ConnectionPort>> get_port(const std::string_view id) const;
    /** Get a port pointer by name */
    std::optional<ConnectionPortRef> get_port_ref(const std::string_view id);
    
    /** \brief Get an iterator over thte ports of this component type */
    port_map_type::const_iterator begin() const { return this->m_ports.begin(); }
    port_map_type::const_iterator end() const { return this->m_ports.end(); }

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
    port_map_type m_ports;
    /* Shape of the component in the workspace */ 
    Footprint m_fp;
    
    friend struct ResourceSerializer<Component>;
    friend class BoardGraph;
};

}

template<>
struct ResourceSerializer<model::Component> {
    using Component = model::Component;
    using IdType = std::string;
    static const std::filesystem::path RESOURCE_DIR;
    using Preloaded = SinglePreload<std::string, 'n', 'a', 'm', 'e'>;
    
    static inline json save(Component& component) {
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
    static inline std::string load_id(const json& json_val) { return json_val.get<std::string>(); }
    static inline std::string save_id(const std::string& id) { return id; }
    template<typename... Resources>
    static inline void load(
        Ref<Component> component,
        const json& json_val,
        GenericResourceManager<Resources...>&,
        const std::string& idref,
        Preloaded& preloaded
    ) {
        component->m_id = std::string_view{idref};
        component->m_name = std::string_view{preloaded.value()};
        json_val.at("footprint").get_to<model::Footprint>(component->m_fp);
        for(const auto& [port_id, port_json] : json_val.at("ports").items()) {
            auto elem = component->m_ports.emplace(port_id, model::ConnectionPort{}).first;
            
            port_json.at("name").get_to(elem->second.m_name);
            port_json.at("pos").get_to(elem->second.m_pt);
            elem->second.m_id = std::string_view{elem->first};
        }
    }
 
};

static_assert(Resource<model::Component>);


