#include "wire.hpp"

std::filesystem::path ConnectorLoader::DIR = "./assets/connectors";

Ref<Connector> ConnectorLoader::load(std::string_view id, const json &json_val, LazyResourceStore &store) {
    (void)store;
    Ref<Connector> component{new Connector{}};
    
    component->m_id = id;
    json_val.at("name").get_to<std::string>(component->m_name);
    return component;
}
