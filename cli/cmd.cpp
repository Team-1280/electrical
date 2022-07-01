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

    struct PurchasedComponent {
        Ref<Component> purchased;
        std::size_t num;
    };
    struct PurchasedConnector {
        Ref<Connector> purchased;
        std::size_t num;
    };
    
    Map<Ref<Component>, std::size_t> components{};
    Map<Ref<Connector>, std::size_t> connectors{};
    bool has_unknown_purchasedata = false;
    std::for_each(
        graph.nodes().begin(),
        graph.nodes().end(),
        [&](auto const& elem) mutable {
            const auto& [_, node] = elem;
            auto existing = components.find(node->type());
            if(existing != components.end()) {
                existing->second += 1;
            } else {
                components.emplace(node->type(), 1);
            }
        }
    );
    std::for_each(
        graph.edges().begin(),
        graph.edges().end(),
        [&](auto const& elem) mutable {
            const auto& [_, node] = elem;
            for(const auto& side : node->connections()) {
                auto existing = connectors.find(side.connector());
                if(existing != connectors.end()) {
                    existing->second += 1;
                } else {
                    connectors.emplace(side.connector(), 1);
                }
            }
        }
    );

    std::uint32_t components_max_cost = 0, components_min_cost = 0;
    std::uint32_t connectors_max_cost = 0, connectors_min_cost = 0;
    switch(format) {
        case OutputFmt::Text: {
            fmt::print(fmt::emphasis::bold, "[Components]\n");
            bool has_purchasedata_for_components = true;
            std::for_each(
                components.cbegin(),
                components.cend(),
                [&](auto const& purchased) {
                    std::uint32_t max_cost = std::numeric_limits<std::uint32_t>::min(), min_cost = std::numeric_limits<std::uint32_t>::max();
                    fmt::print(" - {} x{}: {}\n", 
                        fmt::styled(
                            purchased.first->name(),
                            fmt::emphasis::bold | (purchased.first->purchase_data().has_value() ? 
                                fmt::fg(fmt::color::lime_green) :
                                fmt::fg(fmt::color::pale_violet_red)
                            )
                        ),
                        purchased.second,
                        purchased.first
                            ->purchase_data()
                            .map([&](const auto& data) mutable {
                                for(const auto& item : (std::vector<PurchaseData::Item>const&)data.get()) {
                                    max_cost = std::max(item.cost, max_cost);
                                    min_cost = std::min(item.cost, min_cost);
                                }
                                max_cost *= purchased.second;
                                min_cost *= purchased.second;
                                return fmt::format("${} to ${}", min_cost, max_cost);
                            })
                            .unwrap_or("No purchase data")
                    );
                    if(!purchased.first->purchase_data().has_value()) {
                        has_purchasedata_for_components = false;
                    }
                    components_max_cost += max_cost;
                    components_min_cost += min_cost;
                }
            );

            fmt::print(
                fmt::emphasis::bold | (has_purchasedata_for_components ? fmt::fg(fmt::color::yellow) : fmt::fg(fmt::color::white)),
                "Total cost of components: ${} to ${} {}\n",
                std::min(components_min_cost, std::uint32_t{}),
                components_max_cost,
                has_purchasedata_for_components ? "" : "(!)"
            );

            fmt::print(fmt::emphasis::bold, "[Connectors]\n");
            bool has_purchasedata_for_connectors = true;
            std::for_each(
                connectors.cbegin(),
                connectors.cend(),
                [&](auto const& purchased) {
                    std::uint32_t max_cost = std::numeric_limits<std::uint32_t>::min(), min_cost = std::numeric_limits<std::uint32_t>::max();
                    fmt::print(" - {} x{}: {}\n", 
                        fmt::styled(
                            purchased.first->name(),
                            fmt::emphasis::bold | (purchased.first->purchase_data().has_value() ? 
                                fmt::fg(fmt::color::lime_green) :
                                fmt::fg(fmt::color::pale_violet_red)
                            )
                        ),
                        purchased.second,
                        purchased.first
                            ->purchase_data()
                            .map([&](const auto& data) mutable {
                                for(const auto& item : (std::vector<PurchaseData::Item>const&)data.get()) {
                                    max_cost = std::max(item.cost, max_cost);
                                    min_cost = std::min(item.cost, min_cost);
                                }
                                return fmt::format("${} to ${}", min_cost, max_cost);
                            })
                            .unwrap_or("No purchase data")
                    );
                    if(!purchased.first->purchase_data().has_value()) {
                        has_purchasedata_for_connectors = false;
                    }
                    connectors_max_cost += max_cost;
                    connectors_min_cost += min_cost;
                }
            );

            fmt::print(
                fmt::emphasis::bold | (has_purchasedata_for_connectors ? fmt::fg(fmt::color::yellow) : fmt::fg(fmt::color::white)),
                "Total cost of connectors: ${} to ${} {}\n",
                std::min(connectors_min_cost, std::uint32_t{}),
                connectors_max_cost,
                has_purchasedata_for_connectors ? "" : "(!)"
            );
        } break;
        case OutputFmt::Json: {
            
        } break;
    }
   return 0;
}
