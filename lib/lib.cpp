#include "lib.hpp"
#include "component.hpp"
#include "geom.hpp"
#include "util/log.hpp"
#include "wire.hpp"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <numeric>


std::optional<std::reference_wrapper<const ConnectionPort>> WireEdge::Connection::port() const {
    if(!this->is_floating()) {
        return std::cref(*this->m_port);
    } else {
        return {};
    }
}

Point const& WireEdge::Connection::pos() const {
    if(!this->is_floating()) {
        return this->m_port->pos();
    } else {
        return this->m_pos;
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

Ref<ComponentNode> BoardGraph::component(Ref<Component> type, const std::string& id, Point pos, const std::string_view name) {
    Ref<ComponentNode> node{new ComponentNode()};
    node->m_ty = type;
    node->m_pos = pos;
    node->m_aabb = type->footprint().aabb() + pos;
    auto [elem, _] = this->m_nodes.emplace(id, node);
    node->m_id = std::string_view{elem->first};
    if(!name.empty()) {
        node->m_name = name;
    }
    return node;
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

void BoardGraph::load_node(const std::string& id, const json& root_val) {
    const auto existing = this->get_node(id);
    if(existing.has_value()) {
        return;
    }


    auto [entry, ins] = this->m_nodes.emplace(id, Ref<ComponentNode>{new ComponentNode{}});
    try {
        const auto json_val = root_val.at("nodes").at(id);
        Ref<ComponentNode> node{new ComponentNode{}};
        node->m_name = json_val.at("name").get<std::string>();
        node->m_id = entry->first;
        node->m_ty = this->m_res.try_get<Component>(json_val.at("type").get<std::string_view>());
        node->m_pos = json_val.at("pos").get<Point>();
        for(const auto& conn_json : json_val.at("conns")) {
            ConnectionPortRef port = *node->m_ty->get_port_ref(conn_json.at("port").get<std::string_view>());
            auto edge = this->get_edge(conn_json.at("edge").get<std::string_view>()).unwrap();
            node->m_edges[port] = ComponentNode::EdgeConnection{
                .edge = edge,
                .side = conn_json.at("side").get<WireEdge::Side>()
            };
        }

        node->m_aabb = node->m_ty->m_fp.aabb();
        node->m_aabb.max += node->m_pos;
        node->m_aabb.min += node->m_pos;
        
        entry->second = node;
    } catch(std::exception& e) {
        this->m_nodes.erase(id);
        throw std::runtime_error{fmt::format("Failed to load graph node with ID {}: {}", id, e.what())}; 
    }

}

void BoardGraph::load_edge(const std::string& id, const json& root_val) {
    const auto& existing = this->get_edge(id);
    if(existing.has_value()) {
        return;
    }

    auto [entry, ins] = this->m_edges.emplace(id, Ref<WireEdge>{});

    try {
        const auto json_val = root_val.at("edges").at(id);
        Ref<WireEdge> edge{new WireEdge()};
        edge->m_id = entry->first;
        for(std::size_t i = 0; const auto& conn_json : json_val.at("conns")) {
            if(i >= 2) {
                throw std::runtime_error{"Too many connections for edge, must have exactly two"};
            }
            edge->m_conns[i].m_connector = this->m_res.try_get<Connector>(conn_json.at("connector").get<std::string_view>());
            
            if(conn_json.contains("node") && conn_json.contains("port")) {
                edge->m_conns[i].m_component = this->get_node(conn_json.at("node").get<std::string_view>()).unwrap();
                edge->m_conns[i].m_port = *edge->m_conns[i].m_component.lock()->type()->get_port_ref(conn_json.at("port").get<std::string_view>());
            } else if(conn_json.contains("pos")) {
                conn_json.at("pos").get_to<Point>(edge->m_conns[i].m_pos);
            } else {
                throw std::runtime_error{"Wire edge connection JSON must have either a 'pos' field if the edge is floating or a 'node' and 'port' ID field"};
            }
            i += 1;
        }

        entry->second = edge;
    } catch(std::exception& e) {
        this->m_edges.erase(id);
        throw std::runtime_error{fmt::format("Failed to load graph edge with ID {}: {}", id, e.what())}; 
    }
}

BoardGraph::BoardGraph(std::filesystem::path&& path, bool create, bool save) : m_res{}, m_nodes{}, m_edges{}, m_path{path}, m_save{save} {
    this->m_res.register_loader(new ComponentLoader{});
    this->m_res.register_loader(new ConnectorLoader{});
    if(std::filesystem::exists(path)) {
        std::ifstream json_file{path};
        json root_json;
        try {
            json_file >> root_json;
            from_json(*this, root_json);
        } catch(const std::exception& e) {
            throw std::runtime_error{fmt::format(
                "Failed to read board JSON from {}: {}",
                path.c_str(),
                e.what()
            )};
        }
    } else if(create) {
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
    } else {
        throw std::runtime_error{fmt::format("The graph file at {} does not exist", path.c_str())};
    }
}

BoardGraph::~BoardGraph() {
    if(this->m_save) {
        try {
            std::ofstream savefile{this->m_path};
            savefile << std::setw(4) << this->to_json();
        } catch(const std::exception& e) {
            logger::error("Failed to save board graph to file {}: {}", this->m_path.c_str(), e.what());
        }
    }
}

void BoardGraph::from_json(BoardGraph& self, const json& obj) {
    const auto& nodes = obj.at("nodes");
    const auto& edges = obj.at("edges");
    for(const auto& [id, node] : nodes.items()) {
        self.load_node(id, obj);
    }
    for(const auto& [id, edge] : edges.items()) {
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
        json::object_t edge_json{};
        edge_json.emplace("conns", json::array_t{});
        for(const auto& conn : edge->connections()) {
            json::object_t conn_json{};
            conn_json.emplace("connector", conn.connector()->id());
            if(conn.is_floating()) {
                conn_json.emplace("pos", conn.pos());
            } else {
                conn_json.emplace("node", conn.component().lock()->id());
                conn_json.emplace("port", (*conn.port()).get().id());
            }

            edge_json.at("conns").push_back(std::move(conn_json));
        }
        edges.emplace(edge->id(), std::move(edge_json));
    }

    obj.emplace("nodes", std::move(nodes));
    obj.emplace("edges", std::move(edges));

    return obj;
}
