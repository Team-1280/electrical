#include "lib.hpp"
#include "component.hpp"
#include "geom.hpp"
#include "util/log.hpp"
#include "wire.hpp"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <unordered_set>
#include <numeric>


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
    ComponentNode node{};
    (void)name, (void)type;
    return {};
}

Optional<Ref<ComponentNode>> BoardGraph::get_node(const std::string_view id) const {
    const auto& existing = this->m_nodes.find(id);
    if(existing != this->m_nodes.end()) {
        return existing->second;
    } else {
        return {};
    }
}

Optional<Ref<WireEdge>> BoardGraph::get_edge(const std::string_view id) const {
    const auto& existing = this->m_edges.find(id);
    if(existing != this->m_edges.end()) {
        return existing->second;
    } else {
        return {};
    }
}

void BoardGraph::load_node(const std::string& id, const json::object_t& root_val) {
    const auto existing = this->get_node(id);
    if(existing.has_value()) {
        return;
    }


    auto [entry, ins] = this->m_nodes.emplace(id, Ref<ComponentNode>{new ComponentNode{}});
    try {
        const auto json_val = root_val.at("nodes").at(id).get<json::object_t>();
        Ref<ComponentNode> node{new ComponentNode{}};
        node->m_name = json_val.at("name").get<std::string>();
        node->m_id = entry->first;
        node->m_ty = this->m_res.try_get<Component>(json_val.at("type").get<std::string_view>());
        node->m_pos = json_val.at("pos").get<Point>();
        for(const auto& conn_json : json_val.at("conns")) {
            ConnectionPortRef port = *node->m_ty->get_port_ref(conn_json.at("port").get<std::string_view>());
            auto edge = *this->get_edge(conn_json.at("edge").get<std::string_view>());
            node->m_edges[port] = ComponentNode::EdgeConnection{
                .edge = edge,
                .side = conn_json.at("side").get<WireEdge::Side>()
            };
        }

        entry->second = node;
    } catch(const std::exception& e) {
        this->m_nodes.erase(id);
        logger::error("Failed to load a graph node with id {}: {}", id, e.what());
    }

}

void BoardGraph::load_edge(const std::string& id, const json::object_t& root_val) {
    const auto& existing = this->get_edge(id);
    if(existing.has_value()) {
        return;
    }

    auto [entry, ins] = this->m_edges.emplace(id, Ref<WireEdge>{});

    try {
        const auto json_val = root_val.at("edges").at(id).get<json::object_t>();
        Ref<WireEdge> edge{};
        edge->m_id = entry->first;
        for(std::size_t i = 0; const auto& conn_json : json_val.at("conns")) {
            if(conn_json.is_null() || i >= 2) {
                continue;
            }
            
            edge->m_conns[i].m_component = *this->get_node(conn_json.at("node").get<std::string_view>());
            edge->m_conns[i].m_port = *edge->m_conns[i].m_component.lock()->type()->get_port_ref(conn_json.at("port").get<std::string_view>());
            edge->m_conns[i].m_connector = this->m_res.try_get<Connector>(conn_json.at("connector").get<std::string_view>());
        }

        entry->second = edge;
    } catch(const std::exception& e) {
        this->m_edges.erase(id);
        logger::error(
            "Failed to load a graph edge with id {}: {}",
            id,
            e.what()
        );
    }
}

BoardGraph::BoardGraph(std::filesystem::path&& path) : m_res{}, m_nodes{}, m_edges{}, m_path{path} {
    this->m_res.register_loader(new ComponentLoader{});
    this->m_res.register_loader(new ConnectorLoader{});
    if(std::filesystem::exists(path)) {
        std::ifstream json_file{path};
        json root_json;
        try {
            json_file >> root_json;
            from_json(*this, root_json);
        } catch(const std::exception& e) {
            logger::error(
                "Failed to read board JSON from {}: {}",
                path.c_str(),
                e.what()
            );
            std::exit(-1);
        }
    } else {
        try {
            std::filesystem::create_directories(path.parent_path());
            std::ofstream new_json_file{path};
        } catch(const std::exception& e) {
            logger::error(
                "Failed to create a new save file at {}: {}",
                path.c_str(),
                e.what()
            );
        }
    }
}

BoardGraph::~BoardGraph() {
    try {
        //std::ofstream savefile{this->m_path};
        //savefile << std::setw(4) << this->to_json();
    } catch(const std::exception& e) {
        logger::error("Failed to save board graph to file {}: {}", this->m_path.c_str(), e.what());
    }
}

void BoardGraph::from_json(BoardGraph& self, const json& j) {
    json::object_t obj = j.get<json::object_t>();
    const auto& nodes = obj.at("nodes").get<json::object_t>();
    const auto& edges = obj.at("edges").get<json::object_t>();
    for(const auto& [id, node] : nodes) {
        (void)id;
        self.load_node(id, obj);
    }
    for(const auto& [id, edge] : edges) {
        self.load_edge(id, obj);
    }
}

json BoardGraph::to_json() const {
    json::object_t obj{};
    json::object_t nodes{};
    json::object_t edges{};

    for(const auto& [id, node] : this->m_nodes) {
        json::object_t node_json{};
        node_json.emplace("name", node->name());
        node_json.emplace("type", node->type()->id());
        node_json.emplace("pos", node->pos());
        json::array_t conns{};
        for(const auto& [port, edge] : node->m_edges) {
            json::object_t conn_json{};
            conn_json.emplace("port", port->id());
            conn_json.emplace("edge", edge.edge->id());
            conn_json.emplace("side", edge.side);
            conns.push_back(std::move(conn_json));
        }
        node_json.emplace("conns", std::move(conns));
        
        nodes.emplace(node->id(), std::move(node_json));
    }

    for(const auto& [id, edge] : this->m_edges) {
        json::array_t edge_json{};
        for(const auto& conn : edge->connections()) {
            if(conn.is_floating()) {
                edge_json.push_back(json{nullptr});
                continue;
            }
            json::object_t conn_json{};
            conn_json.emplace("node", conn.component().lock()->id());
            conn_json.emplace("port", (*conn.port()).get().id());
            conn_json.emplace("connector", conn.connector()->id());
            edge_json.push_back(std::move(conn_json));
        }
        edges.emplace(edge->id(), std::move(edge_json));
    }

    obj.emplace("nodes", std::move(nodes));
    obj.emplace("edges", std::move(edges));

    return obj;
}