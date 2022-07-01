#include "cmd.hpp"
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

int BomCommand::run(const BoardGraph &graph, const ArgMatches &args) {
    
}
