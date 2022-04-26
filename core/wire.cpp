#include "wire.hpp"

namespace model {

std::shared_ptr<Connector> Connector::load(
    const json& json_val,
    GenericResourceManagerBase& store,
    const IdType& id,
    GenericStoreEntry<Connector>& entry
) {
    std::shared_ptr<Connector> connector = std::make_shared<Connector>();
    connector->m_id = std::string_view{id};
    connector->m_name = std::string_view{entry.name};

    return connector;
}

json Connector::save(std::shared_ptr<Connector> connector, GenericResourceManagerBase& store) {
    return {
        {"id", connector->m_id},
        {"name", connector->m_name}
    };
}

}
