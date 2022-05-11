#pragma once

#include "component.hpp"
#include "util/hash.hpp"
#include "wire.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
using Optional = std::optional<T>;

namespace _detail {

/** \brief Helper struct enabling an optimization when strings are used as the Id type of a GenericStore */
template<typename Id, typename T>
struct map_type_helper {
    using MapType = std::unordered_map<Id, T>;
};
/** \brief Helper specialization enabling std::string_view's to be passed as IDs instead of string references */
template<typename T>
struct map_type_helper<std::string, T> {
    using MapType = std::unordered_map<std::string, T, StringHasher, std::equal_to<>>; 
};

}

template<typename K, typename V>
using Map = typename _detail::map_type_helper<K, V>::MapType;

/** 
 * \brief Class containing all lazily loaded shared data, like component and wire types
 */
class SharedResourceStore {
public:
    /**
     * \brief Initialize this shared resource store, 
     * loading all resources eagerly from their respective directories
     */
    SharedResourceStore();
    
    /**
     * \brief Attempt to load a component by ID
     * \param id The unique string ID of the component type to load
     * \return An empty optional if the component was not found or deserialization failed,
     * or the deserialized component
     */
    Optional<Ref<Component>> get_component(const std::string_view id);
    
    /**
     * \brief Attempt to load a connector by ID 
     * \param id Unique string identifier of the connector type to load
     * \return An empty optional if the connector's ID was not found or deserialization failed,
     * or the deserialized connector
     */
    Optional<Ref<Connector>> get_connector(const std::string_view id);

private:
    json save_component(const Component& component);

    Map<std::string, Ref<Component>> m_components; ///< A map of component IDs to loaded components
    static const std::filesystem::path COMPONENT_DIR; ///< Directory to load all component files from
    
    Map<std::string, Ref<Connector>> m_connectors; ///< A map of IDs to connector instances
    static const std::filesystem::path CONNECTOR_DIR; ///< Directory to load connectors from
};

using SharedResources = std::shared_ptr<SharedResourceStore>;

