#pragma once

#include "ser/ser.hpp"
#include "ser/store.hpp"
#include <string_view>
#include <memory>


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
     * shared with the SharedResources map key and guranteed to be
     * NULL-terminated
     */
    std::string_view m_id;
    
    /** 
     * \brief Name of this connector type
     */
    std::string m_name;

    friend class ConnectorLoader;
};

class ConnectorLoader : public LazyResourceLoader<Connector> {
public:
    Ref<Connector> load(std::string_view id, const json& json_val, LazyResourceStore& store) override;
    std::filesystem::path const& dir() const noexcept override { return DIR; }

private:
    static std::filesystem::path DIR;
};
