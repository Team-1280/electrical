#pragma once

#include <vector>
#include <string>

#include "currency.hpp"
#include "ser/ser.hpp"
#include "util/optional.hpp"
#include "util/singlevec.hpp"


/** 
 * \brief Wrapper over a list of links that a part can be purchased from
 * \implements ser::JsonSerializable
 * \implements Noneable
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
        /** \brief Recorded cost of the item in USD */
        USD cost;
        /** \brief Create a new item from owned link string and cost */
        Item(std::string&& link, USD cost) : url{std::move(link)}, cost{cost} {}
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
   
    inline constexpr SingleVec<Item>::iterator begin() { return this->m_items.begin(); }
    inline constexpr SingleVec<Item>::iterator end() { return this->m_items.end(); }
    inline constexpr SingleVec<Item>::const_iterator begin() const { return this->m_items.begin(); }
    inline constexpr SingleVec<Item>::const_iterator end() const { return this->m_items.end(); }

    /** \brief Implementing the `Noneable` concept, check if this `PurchaseData` is none */
    inline constexpr bool is_none() const noexcept { return this->m_items.is_none(); }
    /** \brief Implementing the `Noneable` concept, clear all `Item`s from this `PurchaseData` */
    inline constexpr void make_none() { this->m_items.make_none(); }
    
    /** \brief Deserialize a list of `Item`s from the given JSON value */
    static void from_json(PurchaseData& self, json const& json);
    /** \brief Serialize this `PurchaseData` to a JSON value */
    json to_json() const;
private:
    /** \brief List of places to purchase the part */
    SingleVec<Item> m_items;
};

static_assert(ser::JsonSerializable<PurchaseData::Item>);
static_assert(ser::JsonSerializable<PurchaseData>);
static_assert(Noneable<PurchaseData>);
