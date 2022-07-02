#include "cmd.hpp"
#include "fmt/color.h"
#include "util/stackvec.hpp"
#include <limits>
#include <stdexcept>
#include <util/hash.hpp>

BomCommand::BomCommand(Args& args) {
    auto cmd = std::move(Args{"bom", "Generate a Bill of Materials"}
        .with_long_desc("Generate a Bill of Materials by searching all placed components and connectors on the board")
    );

    this->m_outfmt_opt = cmd.arg(Arg {
        .takes_arg = true,
        .arg_name{"format"},
        .short_name{'o'},
        .long_name{"output-format"},
        .short_help{"Select the format that BOM should be presented in [txt,json]"}
    });

    this->id = args.command(std::move(cmd));
}

int BomCommand::run(BoardGraph &graph, const ArgMatches &args) {
    OutputFmt format = args
        .get_arg(this->m_outfmt_opt)
        .map([](auto const& arg) {
            switch(fnv1a_lowercase(arg)) {
                case "txt"_h: return OutputFmt::Text;
                case "json"_h: return OutputFmt::Json;
                default: throw std::runtime_error{fmt::format("Unknown BOM ouput format '{}'", arg)};
            }
        })
        .unwrap_or(OutputFmt::Text);
    
    struct PriceRange {
        std::uint32_t min;
        std::uint32_t max;

        constexpr inline void update(std::uint32_t n) noexcept {
            this->min = std::min(this->min, n);
            this->max = std::max(this->max, n);
        }
        constexpr inline void update(PriceRange const& other) noexcept {
            this->update(other.min);
            this->update(other.max);
        }

        inline std::string to_string() const {
            return this->min == this->max ?
                fmt::format(std::locale("en_US.UTF-8"), "${:L}", this->min) :
                fmt::format(std::locale("en_US.UTF-8"), "${:L} - ${:L}", this->min, this->max);
        }
    };
    struct PurchasedData {
        Optional<PriceRange> price_range{};
        std::size_t num;

        json to_json() const {
            return {
                {
                    "price_range",
                    this
                        ->price_range
                        .map([](const auto& range) {
                            return json::array({range.min, range.max});
                        })
                        .unwrap_or(nullptr)
                },
                {"num", this->num}
            };    
        }
    };
    static const auto get_range = [](PurchaseData const& data) -> Optional<PriceRange> {
        Optional<PriceRange> range{};
        for(const auto& item : data) {
            range = range
                .map([&item](PriceRange& r) { r.update(item.cost); return r; })
                .unwrap_or(PriceRange{.min = item.cost, .max = item.cost});
        }
        return range;
    };

    Map<Ref<Component>, PurchasedData> components{};
    Map<Ref<Connector>, PurchasedData> connectors{};
    Optional<PriceRange> connector_price_range;
    Optional<PriceRange> component_price_range;
    bool all_component_purchasedata = true;
    bool all_connector_purchasedata = true;
    std::for_each(
        graph.nodes().begin(),
        graph.nodes().end(),
        [&](auto const& elem) mutable {
            const auto& [_, node] = elem;
            auto existing = components.find(node->type());
            if(existing != components.end()) {
                existing->second.num += 1;
            } else {
                components.emplace(
                    node->type(),
                    PurchasedData {
                        .price_range = node->type()->purchase_data().map(get_range).flatten(),
                        .num = 1
                    }
                );
            }
        }
    );
    std::for_each(
        graph.edges().begin(),
        graph.edges().end(),
        [&](auto const& elem) mutable {
            const auto& [_, edge] = elem;
            for(const auto& side : edge->connections()) {
                auto existing = connectors.find(side.connector());
                if(existing != connectors.end()) {
                    existing->second.num += 1;
                } else {
                    connectors.emplace(
                        side.connector(),
                        PurchasedData {
                            .price_range = side.connector()->purchase_data().map(get_range).flatten(),
                            .num = 1
                        }
                    );
                }
            }
        }
    );

    std::for_each(
        components.begin(),
        components.end(),
        [&component_price_range,&all_component_purchasedata](auto& elem) {
            auto& purchased = elem.second;
            if(purchased.price_range.has_value()) {
                purchased.price_range.unwrap_unchecked().min *= purchased.num;
                purchased.price_range.unwrap_unchecked().max *= purchased.num;
                component_price_range = component_price_range
                    .map([&purchased](PriceRange& range) {
                        range.min += purchased.price_range.unwrap_unchecked().min;
                        range.max += purchased.price_range.unwrap_unchecked().max;
                        return range;
                    })
                    .unwrap_or(purchased.price_range.unwrap_unchecked());
            } else {
                all_component_purchasedata = false;
            }
        }
    );

    std::for_each(
        connectors.begin(),
        connectors.end(),
        [&connector_price_range,&all_connector_purchasedata](auto& elem) {
            auto& purchased = elem.second;
            if(purchased.price_range.has_value()) {
                purchased.price_range.unwrap_unchecked().min *= purchased.num;
                purchased.price_range.unwrap_unchecked().max *= purchased.num;
                connector_price_range = connector_price_range
                    .map([&purchased](PriceRange& range) {
                        range.min += purchased.price_range.unwrap_unchecked().min;
                        range.max += purchased.price_range.unwrap_unchecked().max;
                        return range;
                    })
                    .unwrap_or(purchased.price_range.unwrap_unchecked());
            } else {
                all_connector_purchasedata = false;
            }
        }
    );

    switch(format) {
        case OutputFmt::Text: {
            fmt::print(fmt::emphasis::bold, "[Components]\n");
            std::for_each(
                components.cbegin(),
                components.cend(),
                [&](auto const& purchased) {
                    fmt::print(" - {} x{}: {}\n", 
                        fmt::styled(
                            purchased.first->name(),
                            fmt::emphasis::bold | (purchased.first->purchase_data().has_value() ? 
                                fmt::fg(fmt::color::lime_green) :
                                fmt::fg(fmt::color::pale_violet_red)
                            )
                        ),
                        purchased.second.num,
                        purchased
                            .second
                            .price_range
                            .map(&PriceRange::to_string)
                            .unwrap_or("[No Data]")
                    );
               }
            );
            
            fmt::print(
                fmt::emphasis::bold | (!all_component_purchasedata ? fmt::fg(fmt::color::yellow) : fmt::fg(fmt::color::white)),
                "Total cost of components: {} {}\n",
                component_price_range
                    .map(&PriceRange::to_string)
                    .unwrap_or("[No Data]"),
                all_component_purchasedata ? "" : "(!)"
            );

            fmt::print(fmt::emphasis::bold, "[Connectors]\n");

            std::for_each(
                connectors.cbegin(),
                connectors.cend(),
                [&](auto const& purchased) {
                    fmt::print(" - {} x{}: {}\n", 
                        fmt::styled(
                            purchased.first->name(),
                            fmt::emphasis::bold | (purchased.first->purchase_data().has_value() ? 
                                fmt::fg(fmt::color::lime_green) :
                                fmt::fg(fmt::color::pale_violet_red)
                            )
                        ),
                        purchased.second.num,
                        purchased
                            .second
                            .price_range
                            .map(&PriceRange::to_string)
                            .unwrap_or("[No Data]")
                    );
                }
            );

            fmt::print(
                fmt::emphasis::bold | (!all_connector_purchasedata ? fmt::fg(fmt::color::yellow) : fmt::fg(fmt::color::white)),
                "Total cost of connectors: {} {}\n",
                connector_price_range
                    .map(&PriceRange::to_string)
                    .unwrap_or("[No Data]"),
                all_connector_purchasedata ? "" : "(!)"
            );
        } break;
        case OutputFmt::Json: {
            json::object_t root{};
            json::object_t components_json{};
            json::object_t connectors_json{};
            for(const auto& [component, data] : components) {
                components_json.emplace(component->id(), data.to_json());
            }
            for(const auto& [connector, data] : connectors) {
                connectors_json.emplace(connector->id(), data.to_json());
            }
            root.emplace("components", std::move(components_json));
            root.emplace("connectors", std::move(connectors_json));
            std::cout << std::setw(2) << root << std::endl;
        } break;
    }
    return 0;
}
