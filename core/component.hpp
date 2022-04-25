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
 * \implements ser::JsonSerializable
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

    static void from_json(ConnectionPort& self, const json& j);
    json to_json() const;
    ConnectionPort() : m_pt{}, m_name{}, m_id{} {}
private:
    /** Location on the component's footprint that this connection port is located */
    Point m_pt;
    /** Name of the port */
    std::string m_name;
    /** Internal ID of this connection port, shared with the parent Component and guranteed to be NULL terminated */
    std::string_view m_id;
    
    friend class ComponentSerializer;
    friend class Component;
};

static_assert(ser::JsonSerializable<ConnectionPort>);

/**
 * \brief A component in the board design with required parameters like
 * footprint
 * \sa ComponentSerializer
 */ 
class Component {
public:
    /** Convert this component to a JSON value */
    json to_json() const;
    
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
    
    friend class ComponentSerializer;
    friend class BoardGraph;
};


/**
 * \brief Class with static methods to serialize components to and from JSON
 * values in a GenericStore
 * \sa GenericStore
 * \implements GenericStoreSerializer
 */
class ComponentSerializer {
public:
    using IdType = std::string;
    static const std::filesystem::path RESOURCE_DIR;
    using Store = GenericStore<Component, ComponentSerializer>;

    static json save(std::shared_ptr<Component>, Store&);
    static std::string load_id(const json&);
    static std::string load_name(const json&);
    static std::shared_ptr<Component> load(
        const json&,
        Store&,
        const std::string&,
        GenericStoreEntry<Component>&
    );
};

using ComponentStore = GenericStore<Component, ComponentSerializer>;
using ComponentRef = ComponentStore::Ref;
using weakComponentRef = ComponentStore::WeakRef;

}
