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
    Connector() : m_id{} {}
    
    /** \brief Get the string ID of this connector type */
    inline constexpr std::string_view id() const { return this->m_id; }
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
    
    friend struct ResourceSerializer<Connector>;
};

}

template<>
struct ResourceSerializer<model::Connector> {
    using Connector = model::Connector;
    using IdType = std::string;
    static const std::filesystem::path RESOURCE_DIR;
    static inline IdType load_id(const json& j) { return j["id"].get<IdType>(); }
    static inline std::string load_name(const json& j) { return j["name"].get<std::string>(); }
    template<Resource... Resources>
    static inline load(
        std::shared_ptr<Connector> connector,
        const json&,
        GenericResourceManager<Resources...>&,
        const IdType& idref,
        ResourceManagerEntry<Connector>& entry
    ) {
        connector->m_id = std::string_view{idref};
        connector->m_name = std::string_view{entry.name};
    }

    template<Resource... Resources>
    static inline json save(std::shared_ptr<Connector> connector, GenericResourceManager<Resources...>&) {
        return {
            {"id", connector->m_id},
            {"name", connector->m_name}
        };
    }
};

static_assert(Resource<model::Connector>);

