#pragma once

#include <vector>
#include <string>

#include "ser/ser.hpp"


/** 
 * \brief Wrapper over a list of links that a part can be purchased from
 * \implements ser::JsonSerializable
 */
struct PurchaseData {
public:
    /**
     * \brief A single item in the list that contains price in USD and a link to 
     * purchase the item
     * \implements ser::JsonSerializable
     */
    struct Item {
    public:
        /** \brief URL of a website page to purchase the item from */
        std::string url;
        /** \brief Last recorded cost of the item in USD, up to $4B */
        uint32_t cost;
        /** \brief Create a new item from owned link string and cost */
        Item(std::string&& link, uint32_t cost) : url{std::move(link)}, cost{cost} {}
        /** \brief Default-construct an item, should only be used for JSON deserialization */
        Item() {}
        
        /**
         * \brief Deserialize an `Item` from the given JSON value
         */
        static void from_json(Item& self, json const& json);
        /** \brief Serialize this `Item` to a JSON value */
        json to_json() const;
    };

    /** \brief Create a new empty `PurchaseData` structure with no links to buy the product */
    inline constexpr PurchaseData() : m_items{} {}
    
    inline constexpr operator std::vector<Item> const&() const { return this->m_items; }
    inline constexpr operator std::vector<Item>&() { return this->m_items; }
    
    /** \brief Deserialize a list of `Item`s from the given JSON value */
    static void from_json(PurchaseData& self, json const& json);
    /** \brief Serialize this `PurchaseData` to a JSON value */
    json to_json() const;
private:
    /** \brief List of places to purchase the part */
    std::vector<Item> m_items;
};

static_assert(ser::JsonSerializable<PurchaseData::Item>);
static_assert(ser::JsonSerializable<PurchaseData>);
