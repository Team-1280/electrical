#include <iostream>
#include <lib.hpp>
#include <util/log.hpp>

#include "args.hpp"
#include "buildopts.h"
#include "fmt/color.h"
#include "geom.hpp"
#include "util/freelist.hpp"


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");

    Args args{"e1280", "Electrical board creator"};
    args
        .with_long_desc("Program to read and manipulate an electrical board represented as an undirected graph")
        .with_version(std::string{BuildOpts::version_str});

    auto help_flag = args.arg(Arg {
        .takes_arg = false,
        .short_name{'h'},
        .long_name{"help"},
        .short_help{"Display extended program usage"}
    });

    auto version_flag = args.arg(Arg {
        .takes_arg = false,
        .short_name{'v'},
        .long_name{"version"},
        .short_help{"Display version message"}
    });

    auto input_file_opt = args.arg(Arg {
        .takes_arg = true,
        .arg_name{"file"},
        .short_name{'i'},
        .long_name{"input"},
        .short_help{"Specify a path to an input file that will be parsed and used for processing"}
    });


    auto bom_cmd = std::move(Args{"bom", "Generate a Bill of Materials"}
        .with_long_desc("Generate a Bill of Materials by searching all placed components and connectors on the board")
    );

    auto bom_outfmt_opt = bom_cmd.arg(Arg {
        .takes_arg = true,
        .arg_name{"format"},
        .short_name{'o'},
        .long_name{"output-format"},
        .short_help{"Select the format that BOM should be presented in"}
    });

    auto bom_subcmd = args.command(std::move(bom_cmd));

    try {
        auto matches = args.matches(argc, argv);
        auto help_match = matches.get(help_flag);
        if(help_match.has_value()) {
            args.print_usage();
            fmt::print("\n\n");
            args.print_help(std::cout, help_match.unwrap().get().long_name);
            return 0;
        } else if(matches.has(version_flag) && args.version().has_value()) {
            fmt::print("e1280 version {}\n", args.version().unwrap().get());
            return 0;
        }

        auto input_file = matches.get_arg(input_file_opt);
        if(!input_file.has_value()) {
            throw std::runtime_error{"No input file given"};
        }

        BoardGraph graph{input_file.unwrap(), false, false};
        auto bom_matches_opt = matches.get_subcommand(bom_subcmd);
        if(bom_matches_opt.has_value()) {
            auto bom_matches = bom_matches_opt.unwrap();
        }
    } catch(const std::exception& e) {
        fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red), "Error: ");
        fmt::print("{}\n", e.what());
        args.print_usage();
        std::cerr << "\n run e1280 --help for more information" << std::endl;
        return -1;
    }

    return 0;
}
