#include "wire.hpp"

namespace model {

std::shared_ptr<Connector> ConnectorSerializer::load(
    const json& json_val,
    Store& store,
    const IdType& id,
    GenericStoreEntry<Connector>& entry
) {
    std::shared_ptr<Connector> connector = std::make_shared<Connector>();
    connector->m_id = std::string_view{id};
    connector->m_name = std::string_view{entry.name};

    return connector;
}

json ConnectorSerializer::save(std::shared_ptr<Connector> connector, Store& store) {
    return {
        {"id", connector->m_id},
        {"name", connector->m_name}
    };
}

}
