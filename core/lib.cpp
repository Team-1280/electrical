#include "lib.hpp"
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

}
