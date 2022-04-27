#pragma once

#include <array>

#include "geom.hpp"
#include "component.hpp"
#include "wire.hpp"
#include "ser.hpp"
#include "store.hpp"
#include "unit.hpp"

using ResourceManager = GenericResourceManager<model::Component, model::Connector>;

namespace model {

class ComponentNode;

/**
 * \brief An edge in the board graph representing a single wire connection between two
 * ports on a component
 * \sa Connector
 */
class WireEdge {
public:     
    /** 
     * A \brief single end of a wire, that may be free-floating (not connected to any component node)
     * or connected to a specific port on a component node
     */
    struct Connection {
    public:
        /** Create a new connection point using a component and pointer to a connection port */
        inline Connection(ResourceManager::WeakRef<ComponentNode> component, ConnectionPort const * port) :
            m_component{component}, m_port{port} {}
    
    private:
        /** \brief Node in the graph that this edge connects to */
        ResourceManager::WeakRef<ComponentNode> m_component;
        /** \brief Pointer to a connection port on the component node's type */
        ConnectionPort const * m_port;
        /** \brief A shared resource pointing to a user-defined connector on this connection point */
        ResourceManager::Ref<Connector> m_connector;

        Connection() : m_component{}, m_port{} {}
        friend class WireEdge;
        friend class BoardGraph;
        friend struct ResourceSerializer<WireEdge>;
    };
   
private:
    /** \brief Components that this wire connects between*/
    std::array<Connection, 2> m_conns;
    
    /** \brief Internal ID number of this wire edge */
    std::size_t m_id;

    WireEdge() : m_conns{} {};

    friend class BoardGraph;
    friend struct ResourceSerializer<WireEdge>;
};

/**
 * \brief A component that has been placed in a BoardGraph with
 * a component type reference and user-entered data
 */
class ComponentNode {
public:
    ComponentNode() : m_ty{}, m_name{}, m_pos{} {}
    
    /** \brief Get the name of this component node */
    inline constexpr const std::string& name() const { return this->m_name; }
    inline constexpr std::size_t id() const { return this->m_id; }
    
    /** \brief Fetch the underlying component type of this node */
    inline ResourceManager::Ref<Component> type() const { return this->m_ty; }
    
    ComponentNode(const std::size_t id) : m_ty{}, m_id{id}, m_name{}, m_pos{} {}

private:
    /** \brief What kind of component this is, shared with other components */
    ResourceManager::Ref<Component> m_ty;
    /** 
     * \brief The internal ID of this component node, it is not a string because it
     * is never presented to the user and is stable between runs of the program
     */
    std::size_t m_id;
    /** \brief User-assigned name of the placed part */
    std::string m_name;
    /** \brief Offset in the workspace from center */
    Point m_pos;
    /** \brief All graph edges connecting this component node to others */
    std::vector<ResourceManager::Ref<WireEdge>> m_wires;

    friend class BoardGraph;
    friend struct ResourceSerializer<ComponentNode>;
};

/**  
 * \brief A graph data structures in which the
 * nodes are `Component`s and the edges are wires
 */
class BoardGraph {
public:
    /** \brief ID number used for nonexistent components when serializing */
    static constexpr const std::size_t NULL_ID = 0;

    /** \brief Deserialize a board graph from a JSON value */
    static void from_json(BoardGraph& self, const json& j);
    /** \brief Serialize this board graph to a JSON value */
    json to_json() const;

    BoardGraph() : m_nodes{}, m_res{} {}    
    inline BoardGraph(BoardGraph&& other) : m_nodes{std::move(other.m_nodes)}, m_res{std::move(other.m_res)} {}
    inline BoardGraph& operator=(BoardGraph&& other) {
        this->m_nodes = std::move(other.m_nodes);
        this->m_res = std::move(other.m_res);
        return *this;
    }
private:
    using compmap_type = std::map<std::size_t, ComponentNodeRef>;
    /** \brief A sparse array of internal IDs to placed components */
    compmap_type m_nodes;
    /** \brief Collection of all loaded component types */
    ResourceManager m_res;
    /** \brief ID counter for component IDs */
    std::size_t m_id_count = 1;
};

}

template<>
struct ResourceSerializer<model::ComponentNode> {
    using ComponentNode = model::ComponentNode;
    using IdType = std::size_t;
    static const std::filesystem::path RESOURCE_DIR;

    static inline IdType load_id(const json& json_val) { return json_val["id"].get<std::size_t>(); }
    static inline std::string load_name(const json& json_val) { return json_val["name"].get<std::string>(); }
    template<Resource... Resources>
    static inline json save(
        std::shared_ptr<ComponentNode> component,
        GenericResourceManager<Resources...>&
    ) {
        json::object_t obj{};
        obj.emplace("name", component->m_name);
        obj.emplace("id", component->m_id);
        obj.emplace("type", component->m_ty->m_id);
        obj.emplace("conns", json::array({}));
        for(const auto& conn : component->m_wires) {
            obj["conns"].push_back(conn.id());
        }

        return obj;
    }

    template<Resource... Resources>
    static inline void load(
        std::shared_ptr<ComponentNode> node,
        const json& json_val,
        GenericResourceManager<Resources...> res,
        const IdType& id,
        const ResourceManagerEntry& entry,
    ) {
        node->m_name = std::string_view{entry.name};
        node->m_id = std::string_view{id};
        node->m_ty = res.try_get<model::Component>(json_val["id"].get<std::string>());
        for(const auto& conn_json : json_val["conns"]) {
            node->m_wires.push_back(res.try_get<WireEdge>(conn_json.get<std::string>));
        }
    }
};

template<>
struct ResourceSerializer<model::WireEdge> {
    using WireEdge = model::WireEdge;
    using IdType = std::size_t;
    static const std::filesystem::path RESOURCE_DIR;

    static inline IdType load_id(const json& json_val) { return json_val["id"].get<std::size_t>(); }
    static inline std::string load_name(const json& json_val) { return ""; }
    template<Resource... Resources>
    static inline json save(
        std::shared_ptr<WireEdge> wire,
        GenericResourceManager<Resource...>&
    ) {
        json::object_t obj{};
        obj.emplace("id", wire->m_id);
        obj.emplace("conns", json::array({}));
        for(const auto& conn : wire->m_conns) {
            if(conn.m_component.expired()) {
                obj["conns"].push_back(nullptr);
            }
            std::shared_ptr<ComponentNode> component = conn.m_component.lock();
            json::object_t conn_obj{};
            conn_obj.emplace("node", component->id());
            conn_obj.emplace("port", conn.m_port->id());
            conn_obj.emplace("connector", conn->m_connector->id());
            obj["conns"].push_back(conn_obj);
        }
    
        return obj;
    }

    template<Resources... Resources>
    static inline void load(

    );
}
