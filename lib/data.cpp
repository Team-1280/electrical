#include "data.hpp"

void PurchaseData::from_json(PurchaseData &self, const json &json) {
    json.get_to<std::vector<Item>>(self.m_items);
}

json PurchaseData::to_json() const {
    return this->m_items;
}

void PurchaseData::Item::from_json(Item &self, const json &json) {
    json.at("price").get_to<uint32_t>(self.cost);
    json.at("url").get_to<std::string>(self.url);
}

json PurchaseData::Item::to_json() const {
    return {
        {"price", this->cost},
        {"url", this->url}
    };
}
