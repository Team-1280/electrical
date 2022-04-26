#pragma once
#include <string_view>
#include <memory>


#include "store.hpp"

namespace model {

/** 
 * \brief Class modelling a single connector with information
 * needed to generate a BOM after modelling the board is complete
 * \sa ConnectorSerializer
 * \implements GenericStoreValue
 */
class Connector {
public:
    using Serializer = class ConnectorSerializer;
    Connector() : m_id{} {}

    
    using IdType = std::string;
    static const std::filesystem::path RESOURCE_DIR;
    static inline IdType load_id(const json& j) { return j["id"].get<IdType>(); }
    static inline std::string load_name(const json& j) { return j["name"].get<std::string>(); }
    static std::shared_ptr<Connector> load(
        const json&,
        GenericResourceManagerBase&,
        const IdType&,
        GenericStoreEntry<Connector>&
    );
    static json save(std::shared_ptr<Connector>, GenericResourceManagerBase&);
private:
    /** 
     * \brief User-created ID string of this connector, 
     * shared with the ConnectorStore's map key and guranteed to be
     * NULL-terminated
     */
    std::string_view m_id;
    
    /** 
     * \brief Name of this connector type, shared with the ConnectorStore's 
     * entry value
     */
    std::string_view m_name;
    
    friend class ConnectorSerializer;
};

static_assert(GenericStoreValue<Connector>);

/** 
 * \brief A GenericStoreSerializer implementation that can
 * deserialize Connector instances from JSON
 * \implements GenericStoreSerializer
 */
class ConnectorSerializer {
public:
    template<typename... Ts>
    using Store = GenericResourceManager<Connector, Ts, Ts...>;

};

static_assert(GenericStoreSerializer<ConnectorSerializer, Connector, ...>);

using ConnectorStore = GenericStore<Connector>;
using ConnectorRef = ConnectorStore::Ref;
using WeakConnectorRef = ConnectorStore::WeakRef;

}
