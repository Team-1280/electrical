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
    return *this->m_res.emplace<ComponentNode>(
        id,
        SinglePreload<std::string>{name},
        std::filesystem::path{""},
        [type](Ref<ComponentNode> val, const uuids::uuid& id, typename ResourceSerializer<ComponentNode>::Preloaded& preload) {
            val->m_ty = type;
            val->m_name = std::string_view{preload.value()};
            val->m_id = uuidref{id.as_bytes()};
        }
    ); 
}

}
