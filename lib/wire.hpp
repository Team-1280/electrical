#pragma once

#include "data.hpp"
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
    Connector() = default;
    
    /** \brief Get the string ID of this connector type */
    inline constexpr const std::string_view id() const { return this->m_id; }
    inline constexpr Optional<std::reference_wrapper<const PurchaseData>> purchase_data() const { return this->m_purchasedata; }
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
    
    /** \brief Where to buy this connector, if any is given */
    Optional<PurchaseData> m_purchasedata;

    friend class ConnectorLoader;
};

class ConnectorLoader : public LazyResourceLoader<Connector> {
public:
    Ref<Connector> load(std::string_view id, const json& json_val, LazyResourceStore& store) override;
    std::filesystem::path const& dir() const noexcept override { return DIR; }

private:
    static std::filesystem::path DIR;
};
