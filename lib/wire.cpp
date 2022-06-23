#include "wire.hpp"

std::filesystem::path ConnectorLoader::DIR = "./assets/connectors";

Ref<Connector> ConnectorLoader::load(std::string_view id, const json &json_val, LazyResourceStore &store) {
    (void)store;
    Ref<Connector> component{new Connector{}};
    
    component->m_id = id;
    json_val.at("name").get_to<std::string>(component->m_name);
    if(json_val.contains("purchase")) {
        json_val.at("purchase").get_to<Optional<PurchaseData>>(component->m_purchasedata);
    }
    return component;
}
