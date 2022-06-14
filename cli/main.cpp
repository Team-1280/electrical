#include <iostream>
#include <lib.hpp>
#include <util/log.hpp>

#include "args.hpp"
#include "buildopts.h"


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");

    Args args{"e1280", "Electrical board creator"};
    args
        .with_long_desc("Program to read and manipulate an electrical board represented as an undirected graph")
        .with_version(std::string{BuildOpts::version_str});
    

    auto help = args.arg(Arg {
        .takes_arg = false,
        .short_name{'h'},
        .long_name{"help"},
        .short_help{"Display a help message"}
    });

    auto version = args.arg(Arg {
        .takes_arg = false,
        .short_name{'v'},
        .long_name{"version"},
        .short_help{"Display program version message"}
    });

    auto input_file = args.arg(Arg {
        .takes_arg = true,
        .arg_name{"file"},
        .short_name{'i'},
        .long_name{"input"},
        .short_help{"Specify a path to an input file that will be parsed and used for processing"}
    });
    
    Args list_ids{"list", "List all node and edge IDs in the graph"};
    auto show_all = list_ids.arg(Arg {
        .takes_arg = false,
        .short_name{'a'},
        .long_name{"all-id"},
        .short_help{"Display all IDs"}
    });

    args.command(std::move(list_ids));
    
    try {
        auto matches = args.matches(argc, argv);
        auto help_match = matches.get(help);
        if(help_match.has_value()) {
            args.print_usage();
            fmt::print("\n\n");
            args.print_help(std::cout, help_match->get().long_name);
        } else if(matches.has(version) && args.version().has_value()) {
            fmt::print("e1280 version {}\n", args.version()->get());
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        args.print_usage();
        std::cerr << "\n run e1280 --help for more information" << std::endl;
    }

    BoardGraph b{"./assets/boards/board.json"};    
    //std::cout << std::setw(4) << b.to_json() << std::endl;

    return 0;
}
