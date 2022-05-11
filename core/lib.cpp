#include "lib.hpp"
#include "store.hpp"
#include "util/log.hpp"
#include "uuid.h"
#include <functional>
#include <unordered_set>

const std::filesystem::path ResourceSerializer<model::ComponentNode>::RESOURCE_DIR = "./assets/boards/nodes";
const std::filesystem::path ResourceSerializer<model::WireEdge>::RESOURCE_DIR = "./assets/boards/edges";

namespace model {

std::optional<std::reference_wrapper<const ConnectionPort>> WireEdge::Connection::port() const {
    if(!this->m_component.expired()) {
        return std::cref(*this->m_port);
    } else {
        return {};
    }
}

void WireEdge::Connection::detach() {
    if(!this->is_floating()) {
        this->m_pos = this->m_component.lock()->pos() + this->m_port->pos();
        this->m_component.lock()->remove_port(this->m_port);
        this->m_component.reset();
    }
}

Optional<std::reference_wrapper<ComponentNode::EdgeConnection>> ComponentNode::connnect_port(ConnectionPortRef port, Ref<WireEdge> edge, const WireEdge::Side side, bool force) {
    auto elem = this->m_ty->get_port(port->name());
    if(!elem.has_value()) {
        return {};
    }
    auto [inserted, good] = this->m_edges.try_emplace(port, edge, side);
    if(!good) {
        if(force) {
            auto existing = this->m_edges.find(port);
            if(existing == this->m_edges.end()) {
                return {};
            }
            existing->second.edge->side(existing->second.side).detach();
            existing->second.edge = edge;
            existing->second.side = side;
            return std::ref(existing->second);
        }
        return {};
    }
    return std::ref(inserted->second);
}

Optional<std::reference_wrapper<ComponentNode::EdgeConnection>> ComponentNode::port(ConnectionPortRef port) {
    auto elem = this->m_edges.find(port);
    if(elem == this->m_edges.end()) {
        return {};
    }

    return std::ref(elem->second);
}

Ref<ComponentNode> BoardGraph::component(Ref<Component> type, const std::string_view name) {
    uuids::uuid id = uuids::uuid_system_generator{}();
    (void)name, (void)type, (void)id;
    return {};
}

Optional<Ref<ComponentNode>> BoardGraph::load_node(const uuid id, const std::filesystem::path& path) {
    auto [entry, ins] = this->m_nodes.emplace(id);

    try {
        std::ifstream file{path};
        json json_val;
        file >> json_val;
        file.close();

        Ref<ComponentNode> node{new ComponentNode{}};
        node->m_name = json_val.at("name").get<std::string>();
        node->m_id = uuidref{entry->first.to_bytes()};
        node->m_ty = *this->m_res->get_component(json_val.at("type").get<std::string_view>());
        for(const auto& conn_json : json_val["conns"]) {
            ConnectionPortRef port = *node->m_ty->get_port_ref(conn_json.at("port").get<std::string_view>());
            node->m_edges[port] = this->load_id(conn_json.get<uuids::uuid>());
        }

        entry->second = node;
    } catch(const std::exception& e) {
        logger::error("Failed to load a graph node from file {} with id {}: {}", path.c_str(), uuids::to_string(id), e.what());
        return {};
    }

}

void BoardGraph::from_json(BoardGraph& self, const json& json_val) {
     
}

BoardGraph::BoardGraph(std::filesystem::path&& path) : m_res{}, m_path{path} {
    
}

}
