#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>

#include <ser/ser.hpp>
#include <geom.hpp>
#include "data.hpp"
#include "ser/store.hpp"
#include "unit.hpp"
#include "util/hash.hpp"


class Component;

/**
 * \brief A structure defining a single connection point on a component
 */
struct ConnectionPort {
public:
    /** Get this connection port's name */
    constexpr inline const std::string& name() const { return this->m_name; }
    /** Get this connection port's ID */
    constexpr inline const std::string& id() const { return this->m_id; }
    /** Get the offset from the component base of this connection port */
    constexpr inline const Point& pos() const { return this->m_pt; }

    inline ConnectionPort(ConnectionPort&& other) = default; 
    inline ConnectionPort& operator=(ConnectionPort&& other) = default;

    ConnectionPort() : m_pt{}, m_name{}, m_id{} {}
private:
    /** Location on the component's footprint that this connection port is located */
    Point m_pt;
    /** Name of the port */
    std::string m_name;
    /** Internal ID of this connection port */
    std::string m_id;
    
    friend class ComponentLoader;
    friend class Component;
};

using ConnectionPortIdx = FreeList<ConnectionPort>::size_type;

/**
 * \brief A component in the board design with required parameters like
 * footprint
 */ 
class Component {
public:
    Component(Component&& other) = default; 
    Component() = default;
    
    /** Get the name of this component */
    inline const std::string_view name() const { return this->m_name; }
    /** Get the user-assigned ID of this component */
    inline constexpr const std::string_view id() const { return this->m_id; }
    /** Get a reference to this component's footprint */
    constexpr inline const Footprint& footprint() const { return this->m_fp; }
    /** Get the mass of this component, if it exists */
    constexpr inline const Optional<std::reference_wrapper<Mass const>> mass() const { return this->m_mass; }
    /** Get a list of places to purchase this component, if any */
    constexpr inline Optional<std::reference_wrapper<PurchaseData const>> purchase_data() const { return this->m_purchasedata; }

    /** Get a port by name, O(1) lookup time */
    Optional<std::reference_wrapper<const ConnectionPort>> get_port(const std::string_view id) const;
    /** \brief Get the port at the given index into the `FreeList` containing all `ConnectionPort`s */
    constexpr Optional<std::reference_wrapper<const ConnectionPort>> get_port(ConnectionPortIdx idx) const;
    /** Get a port pointer by name */
    Optional<ConnectionPortIdx> get_port_idx(const std::string_view id) const;
    
    /** \brief Get an iterator over thte ports of this component type */
    FreeList<ConnectionPort>::const_iterator begin() const { return this->m_ports.begin(); }
    FreeList<ConnectionPort>::const_iterator end() const { return this->m_ports.end(); }
private:
    /* User-facing name of the component type */
    std::string m_name;
    /* ID string of this component, shared with the SharedResources */
    std::string_view m_id;
    /* 
     * Map of IDs to connection points for this component 
     */
    FreeList<ConnectionPort> m_ports;
    /* Shape of the component in the workspace */ 
    Footprint m_fp;
    /** Mass of the component, if any is given */
    Optional<Mass> m_mass;
    /** Where to buy this part and a list of expected costs if any are given */
    Optional<PurchaseData> m_purchasedata;

    friend class BoardGraph;
    friend class ComponentLoader;
};

/**
 * Class dedicated to deserializing `Component`s
 */
class ComponentLoader : public LazyResourceLoader<Component> {
public:
    Ref<Component> load(std::string_view id, const json& json, LazyResourceStore& store) override;
    std::filesystem::path const& dir() const noexcept override { return DIR; }
private:
    static std::filesystem::path DIR;
};
